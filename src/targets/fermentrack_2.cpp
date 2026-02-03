/*
 * Fermentrack 2 Support
 *
 * TiltBridges register themselves with Fermentrack 2, initiated by the user entering the Fermentrack 2
 * information (host, port, and username) in the web interface. The TiltBridge will then send a registration
 * request to Fermentrack 2, which will respond with a Device ID and API key. The TiltBridge does not store the
 * username, but will store the Device ID and API key. From then forward, the Device ID and API key are used to
 * identify the TiltBridge to Fermentrack 2.
 *
 * For now, TiltBridge configuration options cannot be set from within Fermentrack 2, but at some point in the
 * future I may add bidirectional syncing of things like gravity/temperature calibration settings.
 *
 */

#include <ctime>
#include <cstdio>
#include <esp_timer.h>
#include <esp_system.h>

#include <thorlog.h>
#include <ArduinoJson.h>


#include "sendData.h"
#include "version.h"
#include "jsonconfig.h"
#include "send_json_str.h"
#include "targets/fermentrack_2.h"
#include "getGuid.h"
#include "http_calibration.h"
#include "filesystem.h"

#include "tilt/tiltScanner.h"



fermentrackRegErrorT fermentrackRegistrationError;  //<! Error code from last upstream registration attempt
fermentrackStatusErrorT fermentrackStatusError;  //<! Error code from last status attempt
FermentrackMessageFlags fermentrackMessageFlags;

bool register_with_fermentrack_2();
bool send_status_to_fermentrack_2();
bool process_messages_on_fermentrack_2();
void action_fermentrack_messages();

// Calibration sync functions
bool ft2_get_calibration_coefficients(uint8_t color);
bool ft2_set_calibration_coefficients(uint8_t color, double x0, double x1, double x2);
bool ft2_get_calibration_points(uint8_t color);
bool ft2_add_calibration_point(uint8_t color, double sensor_gravity, double measured_gravity);
bool ft2_delete_calibration_point(uint8_t color, double sensor_gravity);
bool ft2_replace_all_calibration_points(uint8_t color);


bool ft2_get_url(char *url, size_t size, const char *path) {
    if(strlen(config.fermentrackHostname) <= 3) {
        Log.error("ft2_get_url: No fermentrack host configured, should skip send.\r\n");
        return false;
    } else if(config.fermentrackPort <= 0 || config.fermentrackPort > 65535) {
        Log.error("ft2_get_url: No upstream port configured, should skip send.\r\n");
        return false;
    }

    if(config.fermentrackPort == 80) {
        snprintf(url, size, "http://%s%s", config.fermentrackHostname, path);
        // TODO - HTTPS support
    } else {
        snprintf(url, size, "http://%s:%d%s", config.fermentrackHostname, config.fermentrackPort, path);
    }
    return true;
}

bool ft2_get_url(char *url, size_t size, const char *path, const char *device_id, const char *api_key) {
    // Used when we need to send the device ID and API key as part of the URL (HTTP_GET)
    if(!ft2_get_url(url, size, path))
        return false;
    
    // Ensure the buffer is large enough for the base URL plus the additional parameters
    size_t base_url_length = strlen(url);
    if (base_url_length + strlen(Fermentrack2SettingsKeys::deviceID) + strlen(device_id) + strlen(Fermentrack2SettingsKeys::apiKey) + strlen(api_key) + 10 > size) {
        // Handle error: buffer not large enough
        return false;
    }

    // Use a temporary buffer to format the URL with parameters
    char temp_url[size];
    snprintf(temp_url, size, "%s?%s=%s&%s=%s", url, Fermentrack2SettingsKeys::deviceID, device_id, Fermentrack2SettingsKeys::apiKey, api_key);
    
    // Copy the formatted URL back into the original buffer
    strncpy(url, temp_url, size);

    return true;
}

bool ft2_is_registered() {
    return (fermentrackRegistrationError == fermentrackRegErrorT::NO_ERROR && strlen(config.fermentrackDeviceID) > 0 && strlen(config.fermentrackAPIKey) > 0);
}




bool dataSendHandler::send_to_fermentrack()
{
    bool result = true;

    if (send_fermentrack && !send_lock)
    {
        // Fermentrack
        send_fermentrack = false;
        send_lock = true;
//        tilt_scanner.deinit();

        // Fermentrack uses a number of functions to process each step of registration/data sending
        register_with_fermentrack_2();
        send_status_to_fermentrack_2();
        process_messages_on_fermentrack_2();


        // Set up for the next send
        data_sender.startTimer(data_sender.fermentrackTimer, FERMENTRACK_DELAY); // Set up subsequent send to Fermentrack
//        tilt_scanner.init();
        send_lock = false;
    }
    return result;
}


/**
 * @brief Registers the device with Fermentrack 2.
 * 
 * This function attempts to register the device with the Fermentrack 2 server. It constructs a JSON payload
 * containing the device's GUID, username or API key, hardware information, and version. The payload is then
 * sent to the Fermentrack 2 server using an HTTP PUT request. If the registration is successful, the device ID
 * and API key are stored in the configuration, and the username is cleared. The updated settings are then saved.
 * 
 * @return true if the registration attempt was made, false otherwise.
 * 
 * @note This function assumes that the handling of tickers/semaphores is done by the caller.
 * 
 * @note The function will skip the registration attempt if the device is already registered or if critical
 *       information (username and API key) is missing.
 * 
 * @note The function will also skip the registration attempt if the URL for the registration endpoint is not set.
 * 
 * @note If the registration is successful, the function will update the configuration with the new device ID and
 *       API key, and clear the username. If the registration fails, an appropriate error code will be set.
 * 
 * @note If the response from the server is invalid, an error code indicating a registration endpoint error will be set.
 */
bool register_with_fermentrack_2() {
    char url[256] = "";
    char payload[512];
    char response[1024];

    // Handling of tickers/semaphores is assumed to be done by whatever calls this function

    // If we've already registered or are missing critical information necessary to register, skip this attempt
    if(ft2_is_registered() || (strlen(config.fermentrackUsername) == 0 && strlen(config.fermentrackAPIKey) == 0) || strlen(config.fermentrackHostname) <= 3)
        return false;
    if(!ft2_get_url(url, sizeof(url), FermentrackAPIEndpoints::registerDevice))
        return false;   // Skip send if the URL is not set

    Log.notice("Registering with Fermentrack 2 at %s\r\n", url);

    {
        JsonDocument doc;

        char guid[20];
        getGuid(guid);

        // Create the JSON payload
        doc["guid"] = guid;
        if(strlen(config.fermentrackUsername) > 0)
            doc[Fermentrack2SettingsKeys::username] = config.fermentrackUsername;
        else
            doc[Fermentrack2SettingsKeys::apiKey] = config.fermentrackAPIKey;
        doc[Fermentrack2SettingsKeys::hardware] = hardware();
        doc[Fermentrack2SettingsKeys::version] = version();

        // Serialize the JSON document
        serializeJson(doc, payload, sizeof(payload));
    }

    sendResult result = http_request(url, httpMethod::HTTP_PUT, payload, response, sizeof(response));

    if(result != sendResult::success) {
        fermentrackRegistrationError = fermentrackRegErrorT::REGISTRATION_ENDPOINT_ERR;
        Log.error("Error registering with Fermentrack 2\r\n");
        return false;
    }

    JsonDocument doc;
    deserializeJson(doc, response);
    Log.verbose("Fermentrack 2 registration response: %s\r\n", response);


    // response = {'success': True, 
    // 'message': 'Device registered', 
    // 'msg_code': 0, 
    // 'device_id': device.id, 
    // 'created': created}
    if(doc["success"].is<bool>()) {
        bool success = doc["success"].as<bool>();

        if(success) {
            // We successfully set the device ID & API key
            fermentrackRegistrationError = fermentrackRegErrorT::NO_ERROR;
            strlcpy(config.fermentrackDeviceID, doc[Fermentrack2SettingsKeys::deviceID].as<const char *>(), sizeof(config.fermentrackDeviceID));
            strlcpy(config.fermentrackAPIKey, doc[Fermentrack2SettingsKeys::apiKey].as<const char *>(), sizeof(config.fermentrackAPIKey));
            config.fermentrackUsername[0] = '\0';  // Clear the username since we now have the apiKey

            // Store the updated settings
            config.save();

            Log.notice("Fermentrack 2 registration successful. Device ID: %s\r\n", config.fermentrackDeviceID);

        } else {
            // We didn't set the device ID (we were unable to register). Set an error code.
            fermentrackRegistrationError = (fermentrackRegErrorT) doc["msg_code"].as<uint8_t>();
            Log.warning("Fermentrack 2 registration failed with error code %d\r\n", fermentrackRegistrationError);
        }
    } else {
        // Invalid response
        fermentrackRegistrationError = fermentrackRegErrorT::REGISTRATION_ENDPOINT_ERR;
    } 

    return true;
}



bool send_status_to_fermentrack_2() {
    char url[256] = "";
    char payload[2048];
    char response[1024];

    // If we aren't registered, we can't send data
    if(!ft2_is_registered())
        return false;
    if(!ft2_get_url(url, sizeof(url), FermentrackAPIEndpoints::status))
        return false;   // Skip send if the URL is not set

    Log.verbose("Sending status to Fermentrack 2 at %s\r\n", url);

    // Construct the JSON payload
    {
        JsonDocument doc;
        char tilt_data[TILT_ALL_DATA_SIZE + 128];

        // Load the Tilt data from the scanner
        JsonDocument tilt_doc;
        tilt_doc = tilt_scanner.tilt_to_json();

        // The API key and Device ID identify this device
        doc[Fermentrack2SettingsKeys::apiKey] = config.fermentrackAPIKey;
        doc[Fermentrack2SettingsKeys::deviceID] = config.fermentrackDeviceID;
        doc["tilts"] = tilt_doc;
        doc[Fermentrack2SettingsKeys::version] = version();

        // Serialize the JSON document
        serializeJson(doc, payload, sizeof(payload));
    }

    Log.info("Sending payload to Fermentrack 2: %s\r\n", payload);

    sendResult result = http_request(url, httpMethod::HTTP_PUT, payload, response, sizeof(response));

    // If we failed to send the data, set an error code
    if(result != sendResult::success) {
        fermentrackStatusError = fermentrackStatusErrorT::STATUS_ENDPOINT_ERR;
        Log.error("Error sending status to Fermentrack 2\r\n");
        return false;
    }

    JsonDocument doc;
    deserializeJson(doc, response);
    Log.verbose("Fermentrack 2 status response: %s\r\n", response);

    if(doc["success"].is<bool>()) {
        bool success = doc["success"].as<bool>();

        if(success) {
            // We successfully sent the device status
            Log.verbose("Fermentrack 2 status successfully sent. Device ID: %s\r\n", config.fermentrackDeviceID);

            // Check if there are messages to retrieve
            if(doc["has_messages"].is<bool>()) {
                fermentrackMessageFlags.hasMessages = doc["has_messages"].as<bool>();
                if(fermentrackMessageFlags.hasMessages) {
                    Log.info("Fermentrack 2 has messages for this device\r\n");
                }
            }
        } else {
            // We didn't succeed (something happened on Fermentrack's end)
            fermentrackStatusError = (fermentrackStatusErrorT) doc["msg_code"].as<uint8_t>();
            Log.warning("Fermentrack 2 status failed to send with error code %d\r\n", fermentrackStatusError);
        }
    } else {
        // Invalid response
        fermentrackStatusError = fermentrackStatusErrorT::STATUS_ENDPOINT_ERR;
    }

    return true;
}

bool process_messages_on_fermentrack_2() {
    char url[256] = "";
    char response[2048];

    // Check if we have messages to retrieve
    if(!fermentrackMessageFlags.hasMessages)
        return false;

    // Clear the flag immediately to prevent re-processing
    fermentrackMessageFlags.hasMessages = false;

    // If we aren't registered, we can't retrieve messages
    if(!ft2_is_registered())
        return false;

    // Build the URL with query parameters for device ID and API key
    if(!ft2_get_url(url, sizeof(url), FermentrackAPIEndpoints::messages, config.fermentrackDeviceID, config.fermentrackAPIKey))
        return false;

    Log.notice("Retrieving messages from Fermentrack 2 at %s\r\n", url);

    // Use HTTP GET to retrieve messages
    sendResult result = http_request(url, httpMethod::HTTP_GET, "", response, sizeof(response));

    if(result != sendResult::success) {
        Log.error("Error retrieving messages from Fermentrack 2\r\n");
        return false;
    }

    JsonDocument doc;
    deserializeJson(doc, response);
    Log.verbose("Fermentrack 2 messages response: %s\r\n", response);

    if(doc["success"].is<bool>()) {
        bool success = doc["success"].as<bool>();

        if(success) {
            // Process the messages
            if(doc["messages"].is<JsonObject>()) {
                JsonObject messages = doc["messages"].as<JsonObject>();
                
                // Set the message flags based on the response
                if(messages["reset_connection"].is<bool>()) {
                    fermentrackMessageFlags.pendingResetConnection = messages["reset_connection"].as<bool>();
                    if(fermentrackMessageFlags.pendingResetConnection) {
                        Log.info("Received reset_connection message from Fermentrack 2\r\n");
                    }
                }
                
                if(messages["restart_device"].is<bool>()) {
                    fermentrackMessageFlags.pendingRestartDevice = messages["restart_device"].as<bool>();
                    if(fermentrackMessageFlags.pendingRestartDevice) {
                        Log.info("Received restart_device message from Fermentrack 2\r\n");
                    }
                }
                
                // Check for per-color sync_calibration flags
                for(uint8_t color = 0; color < 8; color++) {
                    char flag_name[32];
                    snprintf(flag_name, sizeof(flag_name), "sync_calibration_%s", api_color_names[color]);
                    
                    if(messages[flag_name].is<bool>()) {
                        fermentrackMessageFlags.pendingSyncCalibration[color] = messages[flag_name].as<bool>();
                        if(fermentrackMessageFlags.pendingSyncCalibration[color]) {
                            Log.info("Received sync_calibration_%s message from Fermentrack 2\r\n", api_color_names[color]);
                        }
                    }
                }

                // Now PATCH the message flags to false on the remote
                JsonDocument patch_doc;
                char patch_payload[512];

                patch_doc[Fermentrack2SettingsKeys::apiKey] = config.fermentrackAPIKey;
                patch_doc[Fermentrack2SettingsKeys::deviceID] = config.fermentrackDeviceID;
                patch_doc["reset_connection"] = false;
                patch_doc["restart_device"] = false;

                // Clear per-color sync_calibration flags
                for(uint8_t color = 0; color < 8; color++) {
                    char flag_name[32];
                    snprintf(flag_name, sizeof(flag_name), "sync_calibration_%s", api_color_names[color]);
                    patch_doc[flag_name] = false;
                }

                serializeJson(patch_doc, patch_payload, sizeof(patch_payload));

                // Get the base URL for the messages endpoint
                if(ft2_get_url(url, sizeof(url), FermentrackAPIEndpoints::messages)) {
                    char patch_response[256];
                    sendResult patch_result = http_request(url, httpMethod::HTTP_PATCH, patch_payload, patch_response, sizeof(patch_response));

                    if(patch_result == sendResult::success) {
                        Log.verbose("Successfully cleared message flags on Fermentrack 2\r\n");
                    } else {
                        Log.error("Error clearing message flags on Fermentrack 2\r\n");
                    }
                }
            }
        } else {
            Log.warning("Failed to retrieve messages from Fermentrack 2 with error code %d\r\n", doc["msg_code"].as<uint8_t>());
        }
    } else {
        // Invalid response
        Log.error("Invalid response when retrieving messages from Fermentrack 2\r\n");
    }

    // Action the messages we retrieved
    action_fermentrack_messages();

    return true;
}


void action_fermentrack_messages() {
    // Process the reset_connection message first
    if(fermentrackMessageFlags.pendingResetConnection) {
        Log.notice("Resetting Fermentrack 2 connection\r\n");
        
        // Clear all Fermentrack 2 settings
        config.fermentrackHostname[0] = '\0';  // Clear hostname
        config.fermentrackPort = 80;           // Reset to default port
        config.fermentrackUsername[0] = '\0';  // Clear username
        config.fermentrackDeviceID[0] = '\0';  // Clear device ID
        config.fermentrackAPIKey[0] = '\0';    // Clear API key
        
        // Reset the registration error
        fermentrackRegistrationError = fermentrackRegErrorT::NOT_ATTEMPTED_REGISTRATION;
        
        // Save the cleared configuration
        config.save();
        
        // Clear the flag
        fermentrackMessageFlags.pendingResetConnection = false;
        
        Log.notice("Fermentrack 2 connection has been reset\r\n");
    }
    
    // Process per-color sync_calibration messages
    bool anySyncCompleted = false;
    for(uint8_t color = 0; color < 8; color++) {
        if(fermentrackMessageFlags.pendingSyncCalibration[color]) {
            Log.notice("Syncing calibration data for %s Tilt from Fermentrack 2\r\n", tilt_color_names[color]);
            
            // Retrieve calibration coefficients from Fermentrack
            ft2_get_calibration_coefficients(color);
            
            // Retrieve calibration points from Fermentrack
            ft2_get_calibration_points(color);
            
            // Clear the flag for this color
            fermentrackMessageFlags.pendingSyncCalibration[color] = false;
            anySyncCompleted = true;
        }
    }
    
    if(anySyncCompleted) {
        // Save the updated configuration with new calibration coefficients
        config.save();
        Log.notice("Calibration sync from Fermentrack 2 completed\r\n");
    }
    
    // Process the restart_device message last, as it will restart the device
    if(fermentrackMessageFlags.pendingRestartDevice) {
        Log.notice("Restarting device as requested by Fermentrack 2\r\n");

        // ESP-IDF restart function
        esp_restart();
        // Note: The pendingRestartDevice flag will be cleared when the device restarts
    }
}


// Retrieve calibration coefficients from Fermentrack for a specific Tilt
bool ft2_get_calibration_coefficients(uint8_t color) {
    char url[512] = "";
    char response[1024];

    // If we aren't registered, we can't retrieve coefficients
    if(!ft2_is_registered())
        return false;

    // Validate color
    if(color >= TILT_COLORS) {
        Log.error("Invalid Tilt color: %d\r\n", color);
        return false;
    }

    // Build the URL with query parameters for device ID, API key, and color
    char temp_url[256];
    if(!ft2_get_url(temp_url, sizeof(temp_url), FermentrackAPIEndpoints::calibrationCoefficients))
        return false;

    snprintf(url, sizeof(url), "%s?%s=%s&%s=%s&color=%s",
             temp_url,
             Fermentrack2SettingsKeys::deviceID, config.fermentrackDeviceID,
             Fermentrack2SettingsKeys::apiKey, config.fermentrackAPIKey,
             tilt_color_names[color]);

    Log.notice("Retrieving calibration coefficients from Fermentrack 2 at %s\r\n", url);

    // Use HTTP GET to retrieve coefficients
    sendResult result = http_request(url, httpMethod::HTTP_GET, "", response, sizeof(response));

    if(result != sendResult::success) {
        Log.error("Error retrieving calibration coefficients from Fermentrack 2\r\n");
        return false;
    }

    JsonDocument doc;
    deserializeJson(doc, response);
    Log.verbose("Fermentrack 2 calibration coefficients response: %s\r\n", response);

    // Check if successful
    if(doc["success"].is<bool>() && doc["success"].as<bool>()) {
        JsonObject coefficients = doc["coefficients"];
        
        // Update local coefficients if present in response
        if(coefficients["grav_x0"].is<const char*>()) {
            config.tilt_calibration[color].x0 = coefficients["grav_x0"].as<double>();
        }
        if(coefficients["grav_x1"].is<const char*>()) {
            config.tilt_calibration[color].x1 = coefficients["grav_x1"].as<double>();
        }
        if(coefficients["grav_x2"].is<const char*>()) {
            config.tilt_calibration[color].x2 = coefficients["grav_x2"].as<double>();
        }

        Log.notice("Updated calibration coefficients for %s Tilt: x0=%.3f, x1=%.7f, x2=%.7f\r\n", 
                   tilt_color_names[color], 
                   config.tilt_calibration[color].x0,
                   config.tilt_calibration[color].x1,
                   config.tilt_calibration[color].x2);
    } else {
        Log.warning("Failed to retrieve calibration coefficients for %s Tilt\r\n", tilt_color_names[color]);
        return false;
    }

    return true;
}

// Set calibration coefficients on Fermentrack for a specific Tilt
bool ft2_set_calibration_coefficients(uint8_t color, double x0, double x1, double x2) {
    char url[512] = "";
    char payload[512];
    char response[256];

    // If we aren't registered, we can't set coefficients
    if(!ft2_is_registered())
        return false;

    // Validate color
    if(color >= TILT_COLORS) {
        Log.error("Invalid Tilt color: %d\r\n", color);
        return false;
    }

    if(!ft2_get_url(url, sizeof(url), FermentrackAPIEndpoints::calibrationCoefficients))
        return false;

    Log.notice("Setting calibration coefficients on Fermentrack 2 at %s\r\n", url);

    // Construct the JSON payload
    JsonDocument doc;
    doc[Fermentrack2SettingsKeys::apiKey] = config.fermentrackAPIKey;
    doc[Fermentrack2SettingsKeys::deviceID] = config.fermentrackDeviceID;
    doc["color"] = tilt_color_names[color];

    // Format coefficients according to API requirements
    char x0_str[16], x1_str[16], x2_str[16];
    snprintf(x0_str, sizeof(x0_str), "%.3f", x0);
    snprintf(x1_str, sizeof(x1_str), "%.7f", x1);
    snprintf(x2_str, sizeof(x2_str), "%.7f", x2);

    doc["grav_x0"] = x0_str;
    doc["grav_x1"] = x1_str;
    doc["grav_x2"] = x2_str;

    serializeJson(doc, payload, sizeof(payload));

    sendResult result = http_request(url, httpMethod::HTTP_PATCH, payload, response, sizeof(response));

    if(result != sendResult::success) {
        Log.error("Error setting calibration coefficients on Fermentrack 2\r\n");
        return false;
    }

    Log.verbose("Fermentrack 2 calibration coefficients set successfully\r\n");
    return true;
}

// Retrieve calibration points from Fermentrack for a specific Tilt (for future use)
bool ft2_get_calibration_points(uint8_t color) {
    char url[512] = "";
    char response[2048];

    // If we aren't registered, we can't retrieve points
    if(!ft2_is_registered())
        return false;

    // Validate color
    if(color >= TILT_COLORS) {
        Log.error("Invalid Tilt color: %d\r\n", color);
        return false;
    }

    // Build the URL with query parameters for device ID, API key, and color
    char temp_url[256];
    if(!ft2_get_url(temp_url, sizeof(temp_url), FermentrackAPIEndpoints::calibrationPoints))
        return false;

    snprintf(url, sizeof(url), "%s?%s=%s&%s=%s&color=%s",
             temp_url,
             Fermentrack2SettingsKeys::deviceID, config.fermentrackDeviceID,
             Fermentrack2SettingsKeys::apiKey, config.fermentrackAPIKey,
             tilt_color_names[color]);

    Log.notice("Retrieving calibration points from Fermentrack 2 at %s\r\n", url);

    // Use HTTP GET to retrieve points
    sendResult result = http_request(url, httpMethod::HTTP_GET, "", response, sizeof(response));

    if(result != sendResult::success) {
        Log.error("Error retrieving calibration points from Fermentrack 2\r\n");
        return false;
    }

    JsonDocument doc;
    deserializeJson(doc, response);
    Log.verbose("Fermentrack 2 calibration points response: %s\r\n", response);

    // Check if successful and process points
    if(doc["success"].is<bool>() && doc["success"].as<bool>()) {
        JsonArray points = doc["points"];
        
        // If we received at least one calibration point, replace local points
        if(points.size() > 0) {
            // Create filename for calibration data
            char filename[32];
            snprintf(filename, sizeof(filename), "%s/%d-cal.json", CONFIG_DIR, color);
            
            // Clear existing calibration points
            clearCalibrationPoints(color);
            
            // Add each point from Fermentrack
            JsonDocument calDoc;
            JsonArray dataPoints = calDoc.to<JsonArray>();
            
            for(JsonVariant point : points) {
                if(point["sensor_gravity"].is<const char*>() && point["measured_gravity"].is<const char*>()) {
                    double sensorGravity = point["sensor_gravity"].as<double>();
                    double measuredGravity = point["measured_gravity"].as<double>();
                    
                    // Add new calibration point as array [sensorGravity, measuredGravity]
                    JsonArray newPoint = dataPoints.add<JsonArray>();
                    newPoint.add(sensorGravity);
                    newPoint.add(measuredGravity);
                    
                    Log.verbose("Added calibration point: sensor=%.3f, measured=%.3f\r\n", 
                               sensorGravity, measuredGravity);
                }
            }
            
            // Save to file using POSIX API
            FILE *file = fopen(filename, "w");
            if(file) {
                // Serialize to a buffer first, then write
                char jsonBuffer[2048];
                size_t len = serializeJson(calDoc, jsonBuffer, sizeof(jsonBuffer));
                fwrite(jsonBuffer, 1, len, file);
                fclose(file);
                Log.notice("Updated %d calibration points for %s Tilt\r\n",
                           points.size(), tilt_color_names[color]);
            } else {
                Log.error("Failed to save calibration points file\r\n");
                return false;
            }
        } else {
            Log.verbose("No calibration points received from Fermentrack 2\r\n");
        }
    } else {
        Log.warning("Failed to retrieve calibration points for %s Tilt\r\n", tilt_color_names[color]);
        return false;
    }

    return true;
}

// Add calibration point to Fermentrack for a specific Tilt (for future use)
bool ft2_add_calibration_point(uint8_t color, double sensor_gravity, double measured_gravity) {
    char url[512] = "";
    char payload[512];
    char response[256];

    // If we aren't registered, we can't add points
    if(!ft2_is_registered())
        return false;

    // Validate color
    if(color >= TILT_COLORS) {
        Log.error("Invalid Tilt color: %d\r\n", color);
        return false;
    }

    if(!ft2_get_url(url, sizeof(url), FermentrackAPIEndpoints::calibrationPoint))
        return false;

    Log.notice("Adding calibration point to Fermentrack 2 at %s\r\n", url);

    // Construct the JSON payload
    JsonDocument doc;
    doc[Fermentrack2SettingsKeys::apiKey] = config.fermentrackAPIKey;
    doc[Fermentrack2SettingsKeys::deviceID] = config.fermentrackDeviceID;
    doc["color"] = tilt_color_names[color];

    char sensor_str[16], measured_str[16];
    snprintf(sensor_str, sizeof(sensor_str), "%.4f", sensor_gravity);
    snprintf(measured_str, sizeof(measured_str), "%.4f", measured_gravity);

    doc["sensor_gravity"] = sensor_str;
    doc["measured_gravity"] = measured_str;

    serializeJson(doc, payload, sizeof(payload));

    sendResult result = http_request(url, httpMethod::HTTP_POST, payload, response, sizeof(response));

    if(result != sendResult::success) {
        Log.error("Error adding calibration point to Fermentrack 2\r\n");
        return false;
    }

    Log.verbose("Fermentrack 2 calibration point added successfully\r\n");
    return true;
}

// Delete calibration point from Fermentrack for a specific Tilt (for future use)
bool ft2_delete_calibration_point(uint8_t color, double sensor_gravity) {
    char url[512] = "";
    char payload[512];
    char response[256];

    // If we aren't registered, we can't delete points
    if(!ft2_is_registered())
        return false;

    // Validate color
    if(color >= TILT_COLORS) {
        Log.error("Invalid Tilt color: %d\r\n", color);
        return false;
    }

    if(!ft2_get_url(url, sizeof(url), FermentrackAPIEndpoints::calibrationPoint))
        return false;

    Log.notice("Deleting calibration point from Fermentrack 2 at %s\r\n", url);

    // Construct the JSON payload
    JsonDocument doc;
    doc[Fermentrack2SettingsKeys::apiKey] = config.fermentrackAPIKey;
    doc[Fermentrack2SettingsKeys::deviceID] = config.fermentrackDeviceID;
    doc["color"] = tilt_color_names[color];

    char sensor_str[16];
    snprintf(sensor_str, sizeof(sensor_str), "%.4f", sensor_gravity);
    doc["sensor_gravity"] = sensor_str;

    serializeJson(doc, payload, sizeof(payload));

    sendResult result = http_request(url, httpMethod::HTTP_DELETE, payload, response, sizeof(response));

    if(result != sendResult::success) {
        Log.error("Error deleting calibration point from Fermentrack 2\r\n");
        return false;
    }

    Log.verbose("Fermentrack 2 calibration point deleted successfully\r\n");
    return true;
}

// Replace all calibration points on Fermentrack for a specific Tilt color
bool ft2_replace_all_calibration_points(uint8_t color) {
    char url[512] = "";
    char payload[2048];
    char response[1024];

    // If we aren't registered, we can't replace points
    if(!ft2_is_registered())
        return false;

    // Validate color
    if(color >= TILT_COLORS) {
        Log.error("Invalid Tilt color: %d\r\n", color);
        return false;
    }

    if(!ft2_get_url(url, sizeof(url), FermentrackAPIEndpoints::calibrationPoints))
        return false;

    Log.notice("Replacing all calibration points on Fermentrack 2 for %s Tilt at %s\r\n",
               tilt_color_names[color], url);

    // Load calibration points from local storage
    JsonDocument calDoc;
    if(!getCalibrationPoints(color, calDoc)) {
        Log.error("Failed to load local calibration points for %s Tilt\r\n", tilt_color_names[color]);
        return false;
    }

    // Construct the JSON payload
    JsonDocument doc;
    doc[Fermentrack2SettingsKeys::apiKey] = config.fermentrackAPIKey;
    doc[Fermentrack2SettingsKeys::deviceID] = config.fermentrackDeviceID;

    // Convert color index to lowercase color name for API
    doc["color"] = api_color_names[color];

    // Convert local calibration points format to API format
    JsonArray points = doc["points"].to<JsonArray>();
    JsonArray localPoints = calDoc.as<JsonArray>();

    for(JsonVariant localPoint : localPoints) {
        if(localPoint.is<JsonArray>()) {
            JsonArray pointArray = localPoint.as<JsonArray>();
            if(pointArray.size() >= 2 && pointArray[0].is<double>() && pointArray[1].is<double>()) {
                JsonObject apiPoint = points.add<JsonObject>();

                // Format gravity values as strings with appropriate precision
                char sensor_str[16], measured_str[16];
                snprintf(sensor_str, sizeof(sensor_str), "%.4f", pointArray[0].as<double>());
                snprintf(measured_str, sizeof(measured_str), "%.4f", pointArray[1].as<double>());

                apiPoint["sensor_gravity"] = sensor_str;
                apiPoint["measured_gravity"] = measured_str;
            }
        }
    }

    serializeJson(doc, payload, sizeof(payload));

    Log.verbose("Sending %d calibration points to Fermentrack 2\r\n", points.size());

    sendResult result = http_request(url, httpMethod::HTTP_PUT, payload, response, sizeof(response));

    if(result != sendResult::success) {
        Log.error("Error replacing calibration points on Fermentrack 2\r\n");
        return false;
    }

    JsonDocument responseDoc;
    deserializeJson(responseDoc, response);
    Log.verbose("Fermentrack 2 calibration points replacement response: %s\r\n", response);

    // Check if successful
    if(responseDoc["success"].is<bool>() && responseDoc["success"].as<bool>()) {
        Log.notice("Successfully replaced %d calibration points for %s Tilt on Fermentrack 2\r\n", 
                   points.size(), tilt_color_names[color]);
    } else {
        Log.warning("Failed to replace calibration points for %s Tilt on Fermentrack 2. Error code: %d\r\n", 
                   tilt_color_names[color], 
                   responseDoc["msg_code"].is<int>() ? responseDoc["msg_code"].as<int>() : -1);
        return false;
    }

    return true;
}
