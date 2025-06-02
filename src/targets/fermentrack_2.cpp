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


#include <Arduino.h>
#include <ArduinoJson.h>
#include <ctime>
#include "Ticker.h"
#include <ArduinoLog.h>

#include "sendData.h"
#include "version.h"
#include "jsonconfig.h"
#include "send_json_str.h"
#include "targets/fermentrack_2.h"
#include "getGuid.h"

#include "tilt/tiltScanner.h"



fermentrackRegErrorT fermentrackRegistrationError;  //<! Error code from last upstream registration attempt
fermentrackStatusErrorT fermentrackStatusError;  //<! Error code from last status attempt
FermentrackMessageFlags fermentrackMessageFlags;

bool register_with_fermentrack_2();
bool send_status_to_fermentrack_2();
bool process_messages_on_fermentrack_2();
void action_fermentrack_messages();


bool ft2_get_url(char *url, size_t size, const char *path) {
    if(strlen(config.fermentrackHostname) <= 3) {
        Log.error(F("ft2_get_url: No fermentrack host configured, should skip send.\r\n"));
        return false;
    } else if(config.fermentrackPort <= 0 || config.fermentrackPort > 65535) {
        Log.error(F("ft2_get_url: No upstream port configured, should skip send.\r\n"));
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
        fermentrackTicker.once(FERMENTRACK_DELAY, [](){data_sender.send_fermentrack = true;}); // Set up subsequent send to Fermentrack
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
    String payload;
    String response;

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
        serializeJson(doc, payload);
    }

    sendResult result = send_json_str(payload, url, response, httpMethod::HTTP_PUT);

    if(result != sendResult::success) {
        fermentrackRegistrationError = fermentrackRegErrorT::REGISTRATION_ENDPOINT_ERR;
        Log.error("Error registering with Fermentrack 2\r\n");
        return false;
    }

    JsonDocument doc;
    deserializeJson(doc, response);
    Log.verbose("Fermentrack 2 registration response: %s\r\n", response.c_str());


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
    String payload;
    String response;

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
        serializeJson(doc, payload);
    }

    Log.info("Sending payload to Fermentrack 2: %s\r\n", payload.c_str());

    sendResult result = send_json_str(payload, url, response, httpMethod::HTTP_PUT);

    // If we failed to send the data, set an error code
    if(result != sendResult::success) {
        fermentrackStatusError = fermentrackStatusErrorT::STATUS_ENDPOINT_ERR;
        Log.error("Error sending status to Fermentrack 2\r\n");
        return false;
    }

    JsonDocument doc;
    deserializeJson(doc, response);
    Log.verbose("Fermentrack 2 status response: %s\r\n", response.c_str());

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
    String response;

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
    String empty_payload = "";
    sendResult result = send_json_str(empty_payload, url, response, httpMethod::HTTP_GET);

    if(result != sendResult::success) {
        Log.error("Error retrieving messages from Fermentrack 2\r\n");
        return false;
    }

    JsonDocument doc;
    deserializeJson(doc, response);
    Log.verbose("Fermentrack 2 messages response: %s\r\n", response.c_str());

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

                // Now PATCH the message flags to false on the remote
                JsonDocument patch_doc;
                String patch_payload;
                
                patch_doc[Fermentrack2SettingsKeys::apiKey] = config.fermentrackAPIKey;
                patch_doc[Fermentrack2SettingsKeys::deviceID] = config.fermentrackDeviceID;
                patch_doc["reset_connection"] = false;
                patch_doc["restart_device"] = false;
                
                serializeJson(patch_doc, patch_payload);
                
                // Get the base URL for the messages endpoint
                if(ft2_get_url(url, sizeof(url), FermentrackAPIEndpoints::messages)) {
                    String patch_response;
                    sendResult patch_result = send_json_str(patch_payload, url, patch_response, httpMethod::HTTP_PATCH);
                    
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
    
    // Process the restart_device message last, as it will restart the device
    if(fermentrackMessageFlags.pendingRestartDevice) {
        Log.notice("Restarting device as requested by Fermentrack 2\r\n");
        
        // The ESP.restart() function is platform-specific
        ESP.restart();
        // Note: The pendingRestartDevice flag will be cleared when the device restarts
    }
}

