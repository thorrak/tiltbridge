#include <ctime>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <thorlog.h>
#include <ArduinoJson.h>

#include "url_utils.h"
#include "targets/send_json_str.h"

#include "tilt/tiltScanner.h"
#include "mqtt_client.h"  // for init_mqtt()
#include "jsonconfig.h"
#include "http_server.h"
#include "main.h"  // for printMem()
#include "wifi_setup.h"

#include "sendData.h"


dataSendHandler data_sender; // Global data sender

dataSendHandler::dataSendHandler() {}

// Timer callback functions for FreeRTOS software timers
// These are static/free functions that set the semaphore flags
static void legacyFermentrackTimerCallback(TimerHandle_t xTimer) {
    data_sender.send_legacy_fermentrack = true;
}

static void fermentrackTimerCallback(TimerHandle_t xTimer) {
    data_sender.send_fermentrack = true;
}

static void mqttTimerCallback(TimerHandle_t xTimer) {
    data_sender.send_mqtt = true;
}

static void brewStatusTimerCallback(TimerHandle_t xTimer) {
    data_sender.send_brewStatus = true;
}

static void brewfatherTimerCallback(TimerHandle_t xTimer) {
    data_sender.send_brewfather = true;
}

static void brewersFriendTimerCallback(TimerHandle_t xTimer) {
    data_sender.send_brewersFriend = true;
}

static void userTargetTimerCallback(TimerHandle_t xTimer) {
    data_sender.send_userTarget = true;
}

static void gSheetsTimerCallback(TimerHandle_t xTimer) {
    data_sender.send_gSheets = true;
}

static void grainfatherTimerCallback(TimerHandle_t xTimer) {
    data_sender.send_grainfather = true;
}

static void taplistioTimerCallback(TimerHandle_t xTimer) {
    data_sender.send_taplistio = true;
}

static void influxdbTimerCallback(TimerHandle_t xTimer) {
    data_sender.send_influxdb = true;
}

void dataSendHandler::createTimers() {
    // Create all send timers as one-shot timers (pdFALSE)
    // Initial period is 1 tick - we'll change it when we start the timer
    legacyFermentrackTimer = xTimerCreate("LegacyFT", pdMS_TO_TICKS(1000), pdFALSE, nullptr, legacyFermentrackTimerCallback);
    fermentrackTimer = xTimerCreate("Fermentrack", pdMS_TO_TICKS(1000), pdFALSE, nullptr, fermentrackTimerCallback);
    mqttTimer = xTimerCreate("MQTT", pdMS_TO_TICKS(1000), pdFALSE, nullptr, mqttTimerCallback);
    brewStatusTimer = xTimerCreate("BrewStatus", pdMS_TO_TICKS(1000), pdFALSE, nullptr, brewStatusTimerCallback);
    brewfatherTimer = xTimerCreate("Brewfather", pdMS_TO_TICKS(1000), pdFALSE, nullptr, brewfatherTimerCallback);
    brewersFriendTimer = xTimerCreate("BrewersFriend", pdMS_TO_TICKS(1000), pdFALSE, nullptr, brewersFriendTimerCallback);
    userTargetTimer = xTimerCreate("UserTarget", pdMS_TO_TICKS(1000), pdFALSE, nullptr, userTargetTimerCallback);
    gSheetsTimer = xTimerCreate("GSheets", pdMS_TO_TICKS(1000), pdFALSE, nullptr, gSheetsTimerCallback);
    grainfatherTimer = xTimerCreate("Grainfather", pdMS_TO_TICKS(1000), pdFALSE, nullptr, grainfatherTimerCallback);
    taplistioTimer = xTimerCreate("Taplistio", pdMS_TO_TICKS(1000), pdFALSE, nullptr, taplistioTimerCallback);
    influxdbTimer = xTimerCreate("InfluxDB", pdMS_TO_TICKS(1000), pdFALSE, nullptr, influxdbTimerCallback);
}

void dataSendHandler::startTimer(TimerHandle_t timer, uint32_t periodSeconds) {
    if (timer != nullptr) {
        // Stop the timer first (without triggering callback) to ensure clean restart
        xTimerStop(timer, 0);
        // Change period and start - xTimerChangePeriod implicitly starts the timer
        xTimerChangePeriod(timer, pdMS_TO_TICKS(periodSeconds * 1000), 0);
    }
}

void dataSendHandler::init()
{
    init_mqtt();

    // Create all FreeRTOS timers
    createTimers();

    // Schedule first sends with staggered delays to avoid overwhelming the system
    startTimer(legacyFermentrackTimer, 12);      // Schedule first send to Legacy Fermentrack
    startTimer(fermentrackTimer, 10);            // Schedule first send to Fermentrack
    startTimer(mqttTimer, 20);                   // Schedule first send to MQTT
    startTimer(brewStatusTimer, 30);             // Schedule first send to Brew Status
    startTimer(brewfatherTimer, 40);             // Schedule first send to Brewfather
    startTimer(brewersFriendTimer, 50);          // Schedule first send to Brewer's Friend
    startTimer(userTargetTimer, 60);             // Schedule first send to User-defined JSON target
    startTimer(gSheetsTimer, 70);                // Schedule first send to Google Sheets
    startTimer(grainfatherTimer, 80);            // Schedule first send to Grainfather
    startTimer(taplistioTimer, 90);              // Schedule first send to Taplist.io
    startTimer(influxdbTimer, 100);              // Schedule first send to InfluxDB
}

void dataSendHandler::process()
{
    if (is_wifi_connected()) {
        send_to_legacy_fermentrack();
        send_to_fermentrack();
        send_to_bf_and_bf();
        send_to_grainfather();
        send_to_brewstatus();
        send_to_taplistio();
        send_to_google();
        send_to_mqtt();
        send_to_influxdb();
    }
}


bool dataSendHandler::send_to_bf_and_bf()
{
    bool retval = false;
    if (data_sender.send_brewersFriend && !send_lock)
    {
        send_lock = true;
        // Brewer's Friend
        data_sender.send_brewersFriend = false;
        if (strlen(config.brewersFriendKey) > BREWERS_FRIEND_MIN_KEY_LENGTH) {
            Log.verbose("Calling send to Brewer's Friend.\r\n");
            retval = data_sender.send_to_bf_and_bf(BF_MEANS_BREWERS_FRIEND);
            if (retval)
            {
                Log.notice("Completed send to Brewer's Friend.\r\n");
            }
            else
            {
                Log.verbose("Error sending to Brewer's Friend.\r\n");
            }
        }
        startTimer(brewersFriendTimer, BREWERS_FRIEND_DELAY); // Set up subsequent send to Brewer's Friend
        send_lock = false;
    }

    if (data_sender.send_brewfather && !send_lock)
    {
        send_lock = true;
        // Brewfather
        data_sender.send_brewfather = false;
        if (strlen(config.brewfatherKey) > BREWFATHER_MIN_KEY_LENGTH) {
            Log.verbose("Calling send to Brewfather.\r\n");
            retval = data_sender.send_to_bf_and_bf(BF_MEANS_BREWFATHER);
            if (retval)
            {
                Log.notice("Completed send to Brewfather.\r\n");
            }
            else
            {
                Log.verbose("Error sending to Brewfather.\r\n");
            }
        }
        startTimer(brewfatherTimer, BREWFATHER_DELAY); // Set up subsequent send to Brewfather
        send_lock = false;
    }


    if (data_sender.send_userTarget && !send_lock)
    {
        send_lock = true;
        // User Target
        data_sender.send_userTarget = false;
        if (strlen(config.userTargetURL) > USER_TARGET_MIN_URL_LENGTH)
        {
            Log.verbose("Calling send to User Target.\r\n");
            retval = data_sender.send_to_bf_and_bf(BF_MEANS_USER_TARGET);
            if (retval)
            {
                Log.notice("Completed send to User Target.\r\n");
            }
            else
            {
                Log.verbose("Error sending to User Target.\r\n");
            }
        }
        startTimer(userTargetTimer, USER_TARGET_DELAY); // Set up subsequent send to User Target
        send_lock = false;
    }
    return retval;
}

bool dataSendHandler::send_to_bf_and_bf(const uint8_t which_bf)
{
    // This function combines the data formatting for both "BF"s - Brewers
    // Friend & Brewfather. Once the data is formatted, it is dispatched
    // to send_to_url to be sent out.

    bool result = true;
    JsonDocument j;
    char url[128];

    // As this function is being used for both Brewer's Friend and Brewfather,
    // let's determine which we want and set up the URL/API key accordingly.
    if (which_bf == BF_MEANS_BREWFATHER)
    {
        if (strlen(config.brewfatherKey) <= BREWFATHER_MIN_KEY_LENGTH)
        {
            Log.verbose("Brewfather key not populated. Returning.\r\n");
            return false;
        }
        strcpy(url, "http://log.brewfather.net/stream?id=");
        strcat(url, config.brewfatherKey);
    }
    else if (which_bf == BF_MEANS_BREWERS_FRIEND)
    {
        if (strlen(config.brewersFriendKey) <= BREWERS_FRIEND_MIN_KEY_LENGTH)
        {
            Log.verbose("Brewer's Friend key not populated. Returning.\r\n");
            return false;
        }
        strcpy(url, "https://log.brewersfriend.com/stream/");
        strcat(url, config.brewersFriendKey);
    }
    else if (which_bf == BF_MEANS_USER_TARGET)
    {
        if (strlen(config.userTargetURL) <= USER_TARGET_MIN_URL_LENGTH)
        {
            Log.verbose("User target URL not populated. Returning.\r\n");
            return false;
        }
        strcpy(url, config.userTargetURL);
    }
    else
    {
        Log.error("Invalid value of which_bf passed to send_to_bf_and_bf.\r\n");
        return false;
    }

    // Loop through each of the tilt colors cached by tilt_scanner, sending
    // data for each of the active tilts
    tilt_scanner.drop_expired_tilts();
    for(tiltHydrometer & th : tilt_scanner.m_tilt_devices) {
        char gravity[10];
        char temp[6];

        Log.verbose("Tilt loaded with color name: %s\r\n", tilt_color_names[th.m_color]);
        j["name"] = tilt_color_names[th.m_color];
        th.converted_temp(temp, sizeof(temp), true); // Always in Fahrenheit
        j["temp"] = temp;
        j["temp_unit"] = "F";
        th.cal_smooth_gravity_str(gravity, sizeof(gravity));
        j["gravity"] = gravity;
        j["gravity_unit"] = "G";
        j["device_source"] = "TiltBridge";

        char payload_string[BF_SIZE];
        serializeJson(j, payload_string);

        if (http_request(url, httpMethod::HTTP_POST, payload_string) != sendResult::success)
            result = false; // There was an error with the previous send
    }
    return result;
}

bool dataSendHandler::send_to_grainfather()
{
    bool result = true;

    if (send_grainfather && !send_lock)
    {
        // Brew Status
        send_grainfather = false;
        send_lock = true;

        // Loop through each of the tilt colors cached by tilt_scanner, sending
        // data for each of the active tilts
        tilt_scanner.drop_expired_tilts();
        for(tiltHydrometer & th : tilt_scanner.m_tilt_devices) {
            // If there's no Grainfather URL for this color, just continue
            if (strlen(config.grainfatherURL[th.m_color].link) == 0)
                continue;

            Log.verbose("Calling send to Grainfather.\r\n");
            char gravity[10];
            char temp[6];
            JsonDocument j;
            Log.verbose("Tilt loaded with color name: %s\r\n", tilt_color_names[th.m_color]);
            th.converted_temp(temp, sizeof(temp), true); // Always in Fahrenheit
            j["Temp"] = temp;
            j["Unit"] = "F";
            th.cal_smooth_gravity_str(gravity, sizeof(gravity));
            j["SG"] = gravity;

            char payload_string[GF_SIZE];
            serializeJson(j, payload_string);

            if (http_request(config.grainfatherURL[th.m_color].link, httpMethod::HTTP_POST, payload_string) != sendResult::success)
                result = false; // There was an error with the previous send
        }
        startTimer(grainfatherTimer, GRAINFATHER_DELAY); // Set up subsequent send to Grainfather
        send_lock = false;
    }
    return result;
}

bool dataSendHandler::send_to_taplistio()
{
    bool result = true;

    // Check if config.taplistioURL is set, and return if it's not
    if (strlen(config.taplistioURL) <= 10) {
        return false;
    }

    // See if it's our time to send.
    if (!send_taplistio) {
        return false;
    } else if (send_lock) {
        Log.verbose("taplist.io: send lock set.\r\n");
        return false;
    }


    // Since we're using one-shot timers, stop the timer before restarting with new period
    if (taplistioTimer != nullptr) {
        xTimerStop(taplistioTimer, 0);
    }

    // Attempt to send.
    send_taplistio = false;
    send_lock = true;


    tilt_scanner.drop_expired_tilts();

    for(tiltHydrometer & th : tilt_scanner.m_tilt_devices) {
        JsonDocument j;
        char payload_string[192];
        char gravity[10];
        char temp[6];

        j["Color"] = tilt_color_names[th.m_color];
        th.converted_temp(temp, sizeof(temp), true);  // Always in Fahrenheit
        j["Temp"] = temp;
        th.cal_smooth_gravity_str(gravity, sizeof(gravity));
        j["SG"] = gravity;
        j["temperature_unit"] = "F";
        j["gravity_unit"] = "G";
        
        serializeJson(j, payload_string);

        Log.verbose("taplist.io: Sending %s Tilt to %s\r\n", tilt_color_names[th.m_color], config.taplistioURL);

        result = (http_request(config.taplistioURL, httpMethod::HTTP_POST, payload_string) == sendResult::success);
    }

    startTimer(taplistioTimer, config.taplistioPushEvery);
    send_lock = false;
    return result;
}


bool dataSendHandler::send_to_brewstatus()
{
    bool result = true;
    const int payload_size = 512;
    char payload[payload_size];

    if (send_brewStatus && ! send_lock)
    {
        // Brew Status
        send_brewStatus = false;
        send_lock = true;
        if (strlen(config.brewstatusURL) > BREWSTATUS_MIN_URL_LENGTH) {
            Log.verbose("Calling send to Brew Status.\r\n");

            // The payload should look like this when sent to Brewstatus:
            // ('Request payload:', 'SG=1.019&Temp=71.0&Color=ORANGE&Timepoint=43984.33630927084&Beer=Beer&Comment=Comment')
            // BrewStatus ignores Beer, so we just set this to Undefined.
            // BrewStatus will record Comment if it set, but just leave it blank.
            // The Timepoint is Google Sheets time, which is fractional days since 12/30/1899
            // Using https://www.timeanddate.com/date/durationresult.html?m1=12&d1=30&y1=1899&m2=1&d2=1&y2=1970 gives
            // us 25,569 days from the start of Google Sheets time to the start of the Unix epoch.
            // BrewStatus wants local time, so we allow the user to specify a time offset.

            // Loop through each of the tilt colors cached by tilt_scanner, sending data for each of the active tilts
            tilt_scanner.drop_expired_tilts();
            for(tiltHydrometer & th : tilt_scanner.m_tilt_devices) {
                char gravity[10];
                char temp[6];
                th.cal_smooth_gravity_str(gravity, sizeof(gravity));
                th.converted_temp(temp, sizeof(temp), true); // Always in Fahrenheit since we don't send units
                snprintf(payload, payload_size, "SG=%s&Temp=%s&Color=%s&Timepoint=%.11f&Beer=Undefined&Comment=",
                        gravity, temp, tilt_color_names[th.m_color], ((double)std::time(0) + (config.TZoffset * 3600.0)) / 86400.0 + 25569.0);
                
                HttpRequestOptions options;
                options.contentType = content_x_www_form_urlencoded;
                if (http_request(config.brewstatusURL, httpMethod::HTTP_POST, payload, nullptr, 0, options) == sendResult::success) {
                    Log.notice("Completed send to Brew Status.\r\n");
                } else {
                    result = false;
                    Log.verbose("Error sending to Brew Status.\r\n");
                }
            }
        }
        startTimer(brewStatusTimer, config.brewstatusPushEvery); // Set up subsequent send to Brew Status
        send_lock = false;
    }
    return result;
}


bool dataSendHandler::send_to_google()
{
    bool result = true;

    if (send_gSheets && !send_lock) {
        // Google Sheets
        send_gSheets = false;
        send_lock = true;

        JsonDocument payload;
        char payload_string[GSHEETS_JSON];
        JsonDocument retval;
        int numSent = 0;

        // The google sheets handler only fires if we have both a Google Scripts URL to post to, and an email address.
        if (strlen(config.scriptsURL) >= GSCRIPTS_MIN_URL_LENGTH && strlen(config.scriptsEmail) >= GSCRIPTS_MIN_EMAIL_LENGTH) {
            Log.verbose("Checking for any pending Google Sheets pushes.\r\n");
            printMem();

            tilt_scanner.drop_expired_tilts();

            for(tiltHydrometer & th : tilt_scanner.m_tilt_devices) {
                // Check if there is a google sheet name associated with the specific Tilt
                if (strlen(config.gsheets_config[th.m_color].name) > 0) {
                    char gravity[10];
                    char temp[6];

                    // If there's a sheet name saved, then we should send the data
                    if (numSent == 0)
                        Log.notice("Beginning GSheets check-in.\r\n");
                    payload["Beer"] = config.gsheets_config[th.m_color].name;
                    th.converted_temp(temp, sizeof(temp), true); // Always in Fahrenheit
                    payload["Temp"] = temp;
                    th.cal_smooth_gravity_str(gravity, sizeof(gravity));
                    payload["SG"] = gravity;
                    payload["Color"] = tilt_color_names[th.m_color];
                    payload["Comment"] = "";
                    payload["Email"] = config.scriptsEmail; // The gmail email address associated with the script on google
                    payload["tzOffset"] = config.TZoffset;

                    serializeJson(payload, payload_string);
                    payload.clear();

                    Log.verbose("Sending the following payload to Google Sheets (%s):\r\n\t\t%s\r\n",
                               tilt_color_names[th.m_color], payload_string);

                    // Use unified http_request with response buffer to get doclongurl
                    char response[1024];
                    HttpRequestOptions options;
                    options.contentType = content_json;
                    options.skipCertValidation = true;
                    options.timeoutMs = 10000;  // 10 second timeout - Google Scripts can be slow

                    sendResult sendRes = http_request(config.scriptsURL, httpMethod::HTTP_POST,
                                                      payload_string, response, sizeof(response), options);

                    if (sendRes == sendResult::success) {
                        // POST success - parse response for doclongurl
                        Log.verbose("HTTP Response: 200\r\nFull Response:\r\n\t%s\r\n", response);
                        deserializeJson(retval, response);

                        if(strcmp(config.gsheets_config[th.m_color].link, retval["doclongurl"].as<const char *>()) != 0) {
                            Log.verbose("Storing new doclongurl: %s.\r\n", retval["doclongurl"].as<const char *>());
                            strlcpy(config.gsheets_config[th.m_color].link, retval["doclongurl"].as<const char *>(), 255);
                            config.save();
                        }
                        retval.clear();
                        numSent++;
                    } else {
                        // Post generated an error
                        Log.error("Google send to %s Tilt failed. Response:\r\n%s\r\n",
                            tilt_color_names[th.m_color], response);
                        result = false;
                    }

                    vTaskDelay(pdMS_TO_TICKS(100));  // Give some time between requests
                } // Check we have a sheet name for the color
            }

            Log.notice("Submitted %l sheet%s to Google.\r\n", numSent, (numSent== 1) ? "" : "s");

        }
        startTimer(gSheetsTimer, GSCRIPTS_DELAY); // Set up subsequent send to Google Sheets

        send_lock = false;
    }
    return result;
}


bool dataSendHandler::send_to_influxdb()
{
    bool result = true;

    if (send_influxdb && !send_lock)
    {
        send_influxdb = false;
        send_lock = true;

        if (strlen(config.influxdbURL) > INFLUXDB_MIN_URL_LENGTH && strlen(config.influxdbToken) > 0 && strlen(config.influxdbOrg) > 0 && strlen(config.influxdbBucket) > 0) 
        {

            Log.verbose("Calling send to InfluxDB.\r\n");

            // Build the write API URL
            char writeURL[512];
            snprintf(writeURL, sizeof(writeURL), "%s/api/v2/write?org=%s&bucket=%s&precision=s",
                     config.influxdbURL, config.influxdbOrg, config.influxdbBucket);

            // Build line protocol data
            char lineData[2048];
            size_t lineDataLen = 0;
            lineData[0] = '\0';

            tilt_scanner.drop_expired_tilts();
            for(tiltHydrometer & th : tilt_scanner.m_tilt_devices) {
                char gravity[10];
                char temp[6];
                char battery_str[4];

                th.cal_smooth_gravity_str(gravity, sizeof(gravity));
                th.converted_temp(temp, sizeof(temp), false); // Use configured unit
                th.get_weeks_battery(battery_str, sizeof(battery_str));

                // InfluxDB line protocol: measurement,tag1=value1 field1=value1,field2=value2 timestamp
                char line[256];
                int lineLen = snprintf(line, sizeof(line),
                        "tilt,color=%s,device_source=TiltBridge "
                        "gravity=%s,temperature=%s,temp_units=\"%s\",weeks_on_battery=%s\n",
                        tilt_color_names[th.m_color],
                        gravity, temp, config.tempUnit, battery_str);

                // Append to lineData if there's room
                if (lineDataLen + lineLen < sizeof(lineData) - 1) {
                    strlcat(lineData, line, sizeof(lineData));
                    lineDataLen += lineLen;
                }
            }

            if (lineDataLen > 0) {
                // Build authorization header
                char authHeader[256];
                snprintf(authHeader, sizeof(authHeader), "Token %s", config.influxdbToken);

                // Configure request options
                HttpRequestOptions options;
                options.contentType = content_text_plain;
                options.skipCertValidation = true;
                options.authHeader = authHeader;
                options.timeoutMs = 6000;

                // Send the data
                sendResult sendRes = http_request(writeURL, httpMethod::HTTP_POST, lineData, nullptr, 0, options);

                if (sendRes == sendResult::success) {
                    Log.notice("Completed send to InfluxDB.\r\n");
                } else {
                    Log.error("Error sending to InfluxDB\r\n");
                    result = false;
                }
            } else {
                Log.verbose("No Tilt data to send to InfluxDB.\r\n");
            }
        }

        startTimer(influxdbTimer, config.influxdbPushEvery); // Set up subsequent send to InfluxDB
        send_lock = false;
    }
    return result;
}

