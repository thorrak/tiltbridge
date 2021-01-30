//
// Created by John Beeler on 2/18/19.
//

#include "sendData.h"

dataSendHandler data_sender; // Global data sender

MQTTClient mqttClient(256);

// POST Timers
Ticker localTargetTicker;
Ticker brewersFriendTicker;
Ticker brewfatherTicker;
Ticker brewStatusTicker;
Ticker gSheetsTicker;
Ticker mqttTicker;

// POST Semaphores
bool send_localTarget = false;
bool send_brewersFriend = false;
bool send_brewfather = false;
bool send_brewStatus = false;
bool send_gSheets = false;
bool send_mqtt = false;

dataSendHandler::dataSendHandler() {}

void dataSendHandler::init()
{
    init_mqtt();

    // Set up timers
    localTargetTicker.once(20, [](){send_localTarget = true;});      // Schedule first send to Local Target
    brewersFriendTicker.once(50, [](){send_brewersFriend = true;});  // Schedule first send to Brewer's Friend
    brewfatherTicker.once(40, [](){send_brewfather = true;});        // Schedule first send to Brewfather
    brewStatusTicker.once(30, [](){send_brewStatus = true;});        // Schedule first send to Brew Status
    gSheetsTicker.once(70, [](){send_gSheets = true;});              // Schedule first send to Google Sheets
    mqttTicker.once(60, [](){send_mqtt = true;});                    // Schedule first send to MQTT
}

bool dataSendHandler::send_to_localTarget()
{
    bool result = true;

    if (send_localTarget)
    {
        // Local Target
        send_localTarget = false;
        if (WiFiClass::status() == WL_CONNECTED && strlen(config.localTargetURL) >= LOCALTARGET_MIN_URL_LENGTH)
        {
            Log.verbose(F("Calling send to Local Target." CR));

            DynamicJsonDocument j(TILT_ALL_DATA_SIZE + 128);
            char payload[TILT_ALL_DATA_SIZE + 128];

            j["mdns_id"] = config.mdnsID;
            tilt_scanner.tilt_to_json_string(payload, true);
            j["tilts"] = serialized(payload);

            serializeJson(j, payload);

            if (!send_to_url(config.localTargetURL, "", payload, "application/json"))
                result = false; // There was an error with the previous send
        }
        localTargetTicker.once(20, [](){send_localTarget = true;}); // Set up subsequent send to localTarget
    }
    return result;
}

bool send_to_bf_and_bf()
{
    bool retval = false;
    if (send_brewersFriend)
    {
        // Brewer's Friend
        send_brewersFriend = false;
        if (WiFiClass::status() == WL_CONNECTED && strlen(config.brewersFriendKey) > BREWERS_FRIEND_MIN_KEY_LENGTH)
        {
            Log.verbose(F("Calling send to Brewer's Friend." CR));
            retval = data_sender.send_to_bf_and_bf(BF_MEANS_BREWERS_FRIEND);
        }
        brewersFriendTicker.once(50, [](){send_brewersFriend = true;}); // Set up subsequent send to Brewer's Friend
    }

    if (send_brewfather)
    {
        // Brewfather
        send_brewfather = false;
        if (WiFiClass::status() == WL_CONNECTED && strlen(config.brewfatherKey) > BREWFATHER_MIN_KEY_LENGTH)
        {
            Log.verbose(F("Calling send to Brewfather." CR));
            retval = data_sender.send_to_bf_and_bf(BF_MEANS_BREWFATHER);
        }
        brewfatherTicker.once(40, [](){send_brewfather = true;}); // Set up subsequent send to Brewfather
    }
    return retval;
}

bool dataSendHandler::send_to_bf_and_bf(const uint8_t which_bf)
{
    // TODO: (JSON) Come back and tighten this up

    // This function combines the data formatting for both "BF"s - Brewers Friend & Brewfather
    // Once the data is formatted, it is dispatched to send_to_url to be sent out.

    bool result = true;
    StaticJsonDocument<500> j;
    char apiKey[65];
    char url[60];

    // As this function is being used for both Brewer's Friend and Brewfather, let's determine which we want and set up
    // the URL/API key accordingly.
    if (which_bf == BF_MEANS_BREWFATHER)
    {
        if (strlen(config.brewfatherKey) <= BREWFATHER_MIN_KEY_LENGTH)
        {
            Log.verbose(F("Brewfather key not populated. Returning." CR));
            return false;
        }
        strcpy(apiKey, config.brewfatherKey);
        strcpy(url, "http://log.brewfather.net/stream?id=");
        strcat(url, config.brewfatherKey);
    }
    else if (which_bf == BF_MEANS_BREWERS_FRIEND)
    {
        if (strlen(config.brewersFriendKey) <= BREWERS_FRIEND_MIN_KEY_LENGTH)
        {
            Log.verbose(F("Brewer's Friend key not populated. Returning." CR));
            return false;
        }
        strcpy(apiKey, config.brewersFriendKey);
        strcpy(url, "http://log.brewersfriend.com/stream/");
        strcat(url, config.brewersFriendKey);
    }
    else
    {
        Log.error(F("Invalid value of which_bf passed to send_to_bf_and_bf." CR));
        return false;
    }

    // Loop through each of the tilt colors cached by tilt_scanner, sending data for each of the active tilts
    for (uint8_t i = 0; i < TILT_COLORS; i++)
    {
        if (tilt_scanner.tilt(i)->is_loaded())
        {
            Log.verbose(F("Tilt loaded with color name: %s" CR), tilt_scanner.tilt(i)->color_name().c_str());
            j["name"] = tilt_scanner.tilt(i)->color_name();
            j["temp"] = tilt_scanner.tilt(i)->converted_temp(true); // Always in Fahrenheit
            j["temp_unit"] = "F";
            j["gravity"] = tilt_scanner.tilt(i)->converted_gravity(false);
            j["gravity_unit"] = "G";
            j["device_source"] = "TiltBridge";

            char payload_string[500];
            serializeJson(j, payload_string);

            if (!send_to_url(url, apiKey, payload_string, "application/json"))
                result = false; // There was an error with the previous send
        }
    }
    return result;
}

bool dataSendHandler::send_to_brewstatus()
{
    bool result = true;
    const int payload_size = 512;
    char payload[payload_size];

    if (send_brewStatus)
    {
        // Brew Status
        send_brewStatus = false;
        if (WiFiClass::status() == WL_CONNECTED && strlen(config.brewstatusURL) > BREWSTATUS_MIN_URL_LENGTH)
        {
            Log.verbose(F("Calling send to Brew Status." CR));

            // The payload should look like this when sent to Brewstatus:
            // ('Request payload:', 'SG=1.019&Temp=71.0&Color=ORANGE&Timepoint=43984.33630927084&Beer=Beer&Comment=Comment')
            // BrewStatus ignores Beer, so we just set this to Undefined.
            // BrewStatus will record Comment if it set, but just leave it blank.
            // The Timepoint is Google Sheets time, which is fractional days since 12/30/1899
            // Using https://www.timeanddate.com/date/durationresult.html?m1=12&d1=30&y1=1899&m2=1&d2=1&y2=1970 gives
            // us 25,569 days from the start of Google Sheets time to the start of the Unix epoch.
            // BrewStatus wants local time, so we allow the user to specify a time offset.

            // Loop through each of the tilt colors cached by tilt_scanner, sending data for each of the active tilts
            for (uint8_t i = 0; i < TILT_COLORS; i++)
            {
                if (tilt_scanner.tilt(i)->is_loaded())
                {
                    snprintf(payload, payload_size, "SG=%s&Temp=%s&Color=%s&Timepoint=%.11f&Beer=Undefined&Comment=",
                            tilt_scanner.tilt(i)->converted_gravity(false).c_str(),
                            tilt_scanner.tilt(i)->converted_temp(true).c_str(), // Only sending Fahrenheit numbers since we don't send units
                            tilt_scanner.tilt(i)->color_name().c_str(),
                            ((double)std::time(0) + (config.TZoffset * 3600.0)) / 86400.0 + 25569.0);
                    if (!send_to_url(config.brewstatusURL, "", payload, "application/x-www-form-urlencoded"))
                        result = false; // There was an error with the previous send
                }
            }
        }
        brewStatusTicker.once(30, [](){send_brewStatus = true;}); // Set up subsequent send to Brew Status
    }
    return result;
}

bool dataSendHandler::send_to_google()
{
    bool result = true;

    if (send_gSheets)
    {
        // Google Sheets
        send_gSheets = false;

        tilt_scanner.deinit();

        int httpResponseCode;
        int numSent = 0;

        if (strlen(config.scriptsURL) >= GSCRIPTS_MIN_URL_LENGTH && strlen(config.scriptsEmail) >= GSCRIPTS_MIN_EMAIL_LENGTH)
        {
            Log.verbose(F("Checking for any pending Google Sheets pushes." CR));

            for (uint8_t i = 0; i < TILT_COLORS; i++)
            {
                // Loop through each of the tilt colors cached by tilt_scanner
                if (tilt_scanner.tilt(i)->is_loaded())
                {
                    // If there is a color present
                    if (tilt_scanner.tilt(i)->gsheets_beer_name().length() > 0)
                    {
                        if (numSent == 0)
                            Log.notice(F("Beginning GSheets check-in." CR));
                        // If there's a sheet name saved
                        StaticJsonDocument<GSHEETS_JSON> payload;
                        payload["Beer"] = tilt_scanner.tilt(i)->gsheets_beer_name();
                        payload["Temp"] = tilt_scanner.tilt(i)->converted_temp(true); // Always in Fahrenheit
                        payload["SG"] = tilt_scanner.tilt(i)->converted_gravity(false);
                        payload["Color"] = tilt_scanner.tilt(i)->color_name();
                        payload["Comment"] = "";
                        payload["Email"] = config.scriptsEmail; // The gmail email address associated with the script on google
                        payload["tzOffset"] = config.TZoffset;

                        char payload_string[GSHEETS_JSON];
                        serializeJson(payload, payload_string);
                        payload.clear();

                        http.useHTTP10(true);                                   // Turn off chunked transfer encoding to parse the stream
                        http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);  // Follow the 301
                        http.setConnectTimeout(3000);                           // Set 3 second timeout
                        secureClient.setInsecure();                             // Ignore SHA fingerprint

                        if (! http.begin(secureClient, config.scriptsURL))      // Connect secure
                        {
                            Log.error(F("Unable to create secure connection to %s." CR), config.scriptsURL);
                            result = false;
                        } // Failed to open a connection
                        else
                        {
                            Log.verbose(F("Created secure connection to %s." CR), config.scriptsURL);
                            Log.verbose(F("Sending the following payload to Google Sheets (%s):\n\t\t%s" CR), tilt_scanner.tilt(i)->color_name().c_str(), payload_string);

                            http.addHeader(F("Content-Type"), F("application/json"));   // Specify content-type header
                            httpResponseCode = http.POST(payload_string);               // Send the payload

                            if (httpResponseCode == 200)
                            {
                                // POST success
                                StaticJsonDocument<GSHEETS_JSON> retval;
#if (ARDUINO_LOG_LEVEL == 6)
                                Log.verbose(F("JSON Response:" CR));
                                ReadLoggingStream loggingStream(http.getStream(), Serial);
                                deserializeJson(retval, loggingStream);
                                Serial.println();
#else
                                deserializeJson(retval, http.getStream());
#endif
                                switch(i)
                                {
                                    case(0):
                                    {
                                        strlcpy(config.link_red, retval["doclongurl"].as<String>().c_str(), 255);
                                        break;
                                    }
                                    case(1):
                                    {
                                        strlcpy(config.link_green, retval["doclongurl"].as<String>().c_str(), 255);
                                        break;
                                    }
                                    case(2):
                                    {
                                        strlcpy(config.link_black, retval["doclongurl"].as<String>().c_str(), 255);
                                        break;
                                    }
                                    case(3):
                                    {
                                        strlcpy(config.link_purple, retval["doclongurl"].as<String>().c_str(), 255);
                                        break;
                                    }
                                    case(4):
                                    {
                                        strlcpy(config.link_orange, retval["doclongurl"].as<String>().c_str(), 255);
                                        break;
                                    }
                                    case(5):
                                    {
                                        strlcpy(config.link_blue, retval["doclongurl"].as<String>().c_str(), 255);
                                        break;
                                    }
                                    case(6):
                                    {
                                        strlcpy(config.link_yellow, retval["doclongurl"].as<String>().c_str(), 255);
                                        break;
                                    }
                                    case(7):
                                    {
                                        strlcpy(config.link_pink, retval["doclongurl"].as<String>().c_str(), 255);
                                        break;
                                    }
                                    default:
                                    {
                                        break;
                                    }
                                }
                                saveConfig();
                                retval.clear();
                                numSent++;
                            } // Response code = 200
                            else
                            {
                                // Post generated an error
                                Log.error(F("Google send to %s Tilt failed (%d): %s. Response:\n%s" CR),
                                    tilt_scanner.tilt(i)->color_name().c_str(),
                                    httpResponseCode,
                                    http.errorToString(httpResponseCode).c_str(),
                                    http.getString().c_str());
                                result = false;
                            } // Response code != 200
                        } // Good connection
                        http.end();
                        yield();
                    } // Check we have a sheet name for the color
                } // Check scanner is loaded for color
            } // Loop through colors
            Log.notice(F("Submitted %l sheet%s to Google." CR), numSent, (numSent== 1) ? "" : "s");

        }
        gSheetsTicker.once(70, [](){send_gSheets = true;}); // Set up subsequent send to Google Sheets

        tilt_scanner.init();
    }
    return result;
}

void dataSendHandler::init_mqtt()
{
    LCBUrl url;

    if (strcmp(config.mqttBrokerHost, "") != 0 || strlen(config.mqttBrokerHost) != 0)
    {
        if (url.isMDNS(config.mqttBrokerHost))
        {
            Log.verbose(F("Initializing connection to MQTTBroker: %s (%s) on port: %d" CR),
                config.mqttBrokerHost,
                url.getIP(config.mqttBrokerHost).toString().c_str(),
                config.mqttBrokerPort);
        }
        else
        {
            Log.verbose(F("Initializing connection to MQTTBroker: %s on port: %d" CR),
                config.mqttBrokerHost,
                config.mqttBrokerPort);
        }

        if (mqtt_alreadyinit)
        {
            mqttClient.disconnect();
            delay(250);
            if (url.isMDNS(config.mqttBrokerHost))
            {
                mqttClient.setHost(url.getIP(config.mqttBrokerHost), config.mqttBrokerPort);
            }
            else
            {
                mqttClient.setHost(config.mqttBrokerHost, config.mqttBrokerPort);
            }
        }
        else
        {
            if (url.isMDNS(config.mqttBrokerHost))
            {
                mqttClient.begin(url.getIP(config.mqttBrokerHost), config.mqttBrokerPort, mClient);
            }
            else
            {
                mqttClient.begin(config.mqttBrokerHost, config.mqttBrokerPort, mClient);
            }
        }
        mqtt_alreadyinit = true;
        mqttClient.setKeepAlive(config.mqttPushEvery * 1000);
    }
}

void dataSendHandler::connect_mqtt()
{
    if (strlen(config.mqttUsername) > 1)
    {
        mqttClient.connect(config.mdnsID, config.mqttUsername, config.mqttPassword);
    }
    else
    {
        mqttClient.connect(config.mdnsID);
    }
}

bool dataSendHandler::send_to_url(const char *url, const char *apiKey, const char *dataToSend, const char *contentType, bool checkBody, const char* bodyCheck)
{
    // This handles the generic act of sending data to an endpoint
    bool retVal = false;

    if (strlen(dataToSend) > 5 && strlen(url) > 8)
    {
        LCBUrl lcburl;
        lcburl.setUrl(url);

        bool validTarget = false;
        if (lcburl.isMDNS(lcburl.getHost().c_str()))
        {
            // Make sure we can resolve the address
            if (lcburl.getIP(lcburl.getHost().c_str()) != INADDR_NONE)
                validTarget = true;
        }
        else if (lcburl.isValidIP(lcburl.getIP(lcburl.getHost().c_str()).toString().c_str()))
            // We were passed an IP Address
            validTarget = true;
        else
        {
            // If it's not mDNS all we care about is that it's http
            if (lcburl.getScheme() == "http")
                validTarget = true;
        }

        if (validTarget)
        {
            if (lcburl.isMDNS(lcburl.getHost().c_str()))
                // Use the IP address we resolved (necessary for mDNS)
                Log.notice(F("Connecting to: %s at %s on port %l" CR),
                            lcburl.getHost().c_str(),
                            lcburl.getIP(lcburl.getHost().c_str() ).toString().c_str(),
                            lcburl.getPort());
            else
                Log.notice(F("Connecting to: %s on port %l" CR),
                            lcburl.getHost().c_str(),
                            lcburl.getPort());
            urlClient.setNoDelay(true);
            urlClient.setTimeout(10000);
            if (urlClient.connect(lcburl.getIP(lcburl.getHost().c_str()), lcburl.getPort()))
            {
                Log.notice(F("Connected to: %s." CR), lcburl.getHost().c_str());

                // Open POST connection
                if (lcburl.getAfterPath().length() > 0)
                {
                    Log.verbose(F("POST /%s%s HTTP/1.1" CR),
                                lcburl.getPath().c_str(),
                                lcburl.getAfterPath().c_str());
                }
                else
                {
                    Log.verbose(F("POST /%s HTTP/1.1" CR), lcburl.getPath().c_str());
                }
                urlClient.print(F("POST /"));
                urlClient.print(lcburl.getPath().c_str());
                if (lcburl.getAfterPath().length() > 0)
                {
                    urlClient.print(lcburl.getAfterPath().c_str());
                }
                urlClient.println(F(" HTTP/1.1"));

                // Begin headers
                //
                // Host
                Log.verbose(F("Host: %s:%l" CR), lcburl.getHost().c_str(), lcburl.getPort());
                urlClient.print(F("Host: "));
                urlClient.print(lcburl.getHost().c_str());
                urlClient.print(F(":"));
                urlClient.println(lcburl.getPort());
                //
                Log.verbose(F("Connection: close" CR));
                urlClient.println(F("Connection: close"));
                // Content
                Log.verbose(F("Content-Length: %l" CR), strlen(dataToSend));
                urlClient.print(F("Content-Length: "));
                urlClient.println(strlen(dataToSend));
                // Content Type
                Log.verbose(F("Content-Type: %s" CR), contentType);
                urlClient.print(F("Content-Type: "));
                urlClient.println(contentType);
                // API Key
                if (strlen(apiKey) > 2)
                {
                        Log.verbose(F("X-API-KEY: %s" CR), apiKey);
                        urlClient.print(F("X-API-KEY: "));
                        urlClient.println(apiKey);
                }
                // Terminate headers with a blank line
                Log.verbose(F("End headers." CR));
                urlClient.println();
                //
                // End Headers

                // Post JSON
                urlClient.println(dataToSend);
                // Check the HTTP status (should be "HTTP/1.1 200 OK")
                char status[32] = {0};
                urlClient.readBytesUntil('\r', status, sizeof(status));
                urlClient.stop();
                Log.verbose(F("Status: %s" CR), status);
                if (strcmp(status + 9, "200 OK") == 0)
                {
                    if (checkBody == true) // We can do additional checks here
                    {
                        // Check body
                        String response = String(status);
                        if (response.indexOf(bodyCheck) >= 0)
                        {
                            Log.verbose(F("Response body ok." CR));
                            retVal = true;
                        }
                        else
                        {
                            Log.error(F("Unexpected body content: %s" CR), response.c_str());
                            retVal = false;
                        }
                    }
                    else
                    {
                        Log.notice(F("Post to %s was successful." CR), lcburl.getHost().c_str());
                        retVal = true;
                    }
                }
                else
                {
                    Log.error(F("Unexpected status: %s" CR), status);
                    retVal = false;
                }
            }
            else
            {
                Log.warning(F("Connection failed, Host: %s, Port: %l (Err: %d)" CR),
                            lcburl.getHost().c_str(), lcburl.getPort(), urlClient.connected());
                retVal = false;
            }
        }
        else
        {
            Log.error(F("Invalid target: %s." CR), url);
        }
    }
    else
    {
        Log.notice(F("No URL provided, or no data to send." CR));
        retVal = false;
    }
    return retVal;
}

bool dataSendHandler::send_to_mqtt()
{
    // TODO: (JSON) Come back and tighten this up
    bool result = false;
    StaticJsonDocument<1500> payload;
    mqttClient.loop();

    if (send_mqtt)
    {
        // MQTT
        send_mqtt = false;
        if (strcmp(config.mqttBrokerHost, "") != 0 || strlen(config.mqttBrokerHost) != 0)
        {
            Log.verbose(F("Publishing available results to MQTT Broker." CR));
            // Function sends three payloads with the first two designed to support autodiscovery and configuration
            // on Home Assistant.
            // General payload formatted as json when sent to mqTT:
            //{"Color":"Black","SG":"1.0180","Temp":"73.0","fermunits":"SG","tempunits":"F","timeStamp":1608745710}
            //
            // Loop through each of the tilt colors cached by tilt_scanner, sending data for each of the active tilts
            for (uint8_t i = 0; i < TILT_COLORS; i++)
            {
                if (tilt_scanner.tilt(i)->is_loaded())
                {
                    char tilt_topic[50] = {'\0'};
                    snprintf(tilt_topic, 50, "%s/tilt_%s",
                            config.mqttTopic,
                            tilt_scanner.tilt(i)->color_name().c_str());

                    for (uint8_t j = 0; j < 3; j++)
                    {
                        char m_topic[90] = {'\0'};
                        char tilt_name[35] = {'\0'};
                        char unit[10] = {'\0'};
                        bool retain = false;
                        switch (j)
                        {
                        case 0: //Home Assistant Config Topic for Temperature
                            sprintf(m_topic, "homeassistant/sensor/%s_tilt_%sT/config",
                                    config.mqttTopic,
                                    tilt_scanner.tilt(i)->color_name().c_str());
                            payload["dev_cla"] = "temperature";
                            strcat(unit, "\u00b0");
                            strcat(unit, config.tempUnit);
                            payload["unit_of_meas"] = unit;
                            payload["ic"] = "mdi:thermometer";
                            payload["stat_t"] = tilt_topic;
                            strcat(tilt_name, "Tilt Temperature - ");
                            strcat(tilt_name, tilt_scanner.tilt(i)->color_name().c_str());
                            payload["name"] = tilt_name;
                            payload["val_tpl"] = "{{value_json.Temp}}";
                            retain = true;
                            break;
                        case 1: //Home Assistant Config Topic for Sp Gravity
                            sprintf(m_topic, "homeassistant/sensor/%s_tilt_%sG/config",
                                    config.mqttTopic,
                                    tilt_scanner.tilt(i)->color_name().c_str());
                            //payload["dev_cla"] = "None";
                            payload["unit_of_meas"] = "SG";
                            //payload["ic"] = "";
                            payload["stat_t"] = tilt_topic;
                            strcat(tilt_name, "Tilt Specific Gravity - ");
                            strcat(tilt_name, tilt_scanner.tilt(i)->color_name().c_str());
                            payload["name"] = tilt_name;
                            payload["val_tpl"] = "{{value_json.SG}}";
                            retain = true;
                            break;
                        case 2: //General payload with sensor data
                            strcat(m_topic, tilt_topic);
                            char current_grav[8] = {'\0'};
                            char current_temp[5] = {'\0'};
                            strcpy(current_grav, tilt_scanner.tilt(i)->converted_gravity(false).c_str());
                            strcpy(current_temp, tilt_scanner.tilt(i)->converted_temp(false).c_str());
                            payload["Color"] = tilt_scanner.tilt(i)->color_name();
                            payload["timeStamp"] = (int)std::time(0);
                            payload["fermunits"] = "SG";
                            payload["SG"] = current_grav;
                            payload["Temp"] = current_temp;
                            payload["tempunits"] = config.tempUnit;
                            retain = false;
                            break;
                        }
                        char payload_string[300] = {'\0'};
                        serializeJson(payload, payload_string);

                        Log.verbose(F("Topic: %s" CR), m_topic);
                        Log.verbose(F("Message: %s" CR), payload_string);

                        if (!mqttClient.connected() && j == 0)
                        {
                            Log.verbose(F("MQTT disconnected. Attempting to reconnect to MQTT Broker" CR));
                            connect_mqtt();
                        }

                        result = mqttClient.publish(m_topic, payload_string, retain, 0);
                        delay(10);

                        Log.verbose(F("Publish success: %T" CR), result);
                        payload.clear();
                    }
                }
            }
        }
        mqttTicker.once(config.mqttPushEvery, [](){send_mqtt = true;});   // Set up subsequent send to MQTT
    }
    return result;
}
