//
// Created by John Beeler on 2/18/19.
//

#include "sendData.h"

dataSendHandler data_sender; // Global data sender

WiFiClient wClient;
MQTTClient mqttClient(256);

dataSendHandler::dataSendHandler()
{
    send_to_localTarget_at = 20 * 1000;    // Trigger first send to Fermentrack 20 seconds out
    send_to_brewstatus_at = 40 * 1000;     // Trigger first send to BrewStatus 40 seconds out
    send_to_brewfather_at = 50 * 1000;     // Trigger first send to Brewfather 50 seconds out
    send_to_brewers_friend_at = 55 * 1000; // Trigger first send to Brewer's Friend 55 seconds out
    send_to_mqtt_at = 60 * 1000;           // Trigger first send to MQTT 60 seconds out
    send_to_google_at = 70 * 1000;         // Trigger first send to Google Sheets 70 seconds out
#ifdef ENABLE_TEST_CHECKINS
    send_checkin_at = 35 * 1000; // If we have send_checkins enabled (this is a testing thing!) send at 35 seconds
#endif

    mqtt_alreadyinit = false;
}

void dataSendHandler::setClock()
{
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    time_t nowSecs = time(nullptr);
    while (nowSecs < 8 * 3600 * 2)
    {
        delay(500);
        yield();
        nowSecs = time(nullptr);
    }

    struct tm timeinfo;
    gmtime_r(&nowSecs, &timeinfo);
}

void dataSendHandler::init()
{
    setClock();
}

void dataSendHandler::init_mqtt()
{
    //if (strlen(config.mqttBrokerHost) > IP_MIN_STRING_LENGTH) // TODO:  Replace this check
    if (true)
    {
        Log.verbose(F("Initializing Connection to MQTTBroker at IP: %s on port: %d" CR), config.mqttBrokerHost, config.mqttBrokerPort);
        mqttClient.setKeepAlive(config.mqttPushEvery * 1000);

        if (mqtt_alreadyinit)
        {
            mqttClient.disconnect();
            delay(250);
            mqttClient.setHost(config.mqttBrokerHost, config.mqttBrokerPort);
        }
        else
        {
            mqttClient.begin(config.mqttBrokerHost, config.mqttBrokerPort, wClient);
        }
        mqtt_alreadyinit = true;
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

bool dataSendHandler::send_to_localTarget()
{
    // TODO: (JSON) Come back and tighten this up
    bool result = true;
    DynamicJsonDocument j(1550);
    // This should look like this when sent to Fermentrack:
    // {
    //   'mdns_id': 'mDNS ID Goes Here',
    //   'tilts': {'color': 'Purple', 'temp': 74, 'gravity': 1.043},
    //            {'color': 'Orange', 'temp': 66, 'gravity': 1.001}
    // }

    char payload[1600];

    j["mdns_id"] = config.mdnsID;
    tilt_scanner.tilt_to_json_string(payload, true);
    j["tilts"] = serialized(payload);

    serializeJson(j, payload);

    if (!send_to_url(config.localTargetURL, "", payload, "application/json"))
        result = false; // There was an error with the previous send

    return result;
}

bool dataSendHandler::send_to_brewstatus()
{
    bool result = true;
    const int payload_size = 512;
    char payload[payload_size];

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

    return result;
}

// For sending data to Google Scripts, we have to use secure_client but otherwise we're doing the same thing as before.
bool dataSendHandler::send_to_url_https(const char *url, const char *apiKey, const char *dataToSend, const char *contentType)
{
    // This handles the generic act of sending data to an endpoint
    bool result = false;

    if (strlen(dataToSend) > 5)
    {
        Log.verbose(("[HTTPS] Sending data to: %s." CR), url);
        Log.verbose(F("Data to send: %s" CR), dataToSend);
        Log.verbose(F("[HTTPS] Pre-deinit RAM left %d" CR), esp_get_free_heap_size());

        // We're severely memory starved. Deinitialize bluetooth and free the related memory
        // NOTE - This is not strictly true under NimBLE. Deinit now only waits for a scan to complete
        tilt_scanner.deinit();
        yield();

        Log.verbose(F("[HTTPS] Post-deinit RAM left %d" CR), esp_get_free_heap_size());
        Log.verbose(F("[HTTPS] Calling SWR::send_with_redirects." CR));

        SecureWithRedirects SWR(url, apiKey, dataToSend, contentType);
        result = SWR.send_with_redirects();
        SWR.end();
        yield();
        Log.verbose(F("[HTTPS] Post-SWR RAM left %d" CR), esp_get_free_heap_size());

        tilt_scanner.init();
        yield();
        Log.verbose(F("[HTTPS] Post-Reinit RAM left %d" CR), esp_get_free_heap_size());
    }
    return result;
}

bool dataSendHandler::send_to_google()
{
    // TODO: (JSON) Come back and tighten this up
    HTTPClient http;
    StaticJsonDocument<750> payload;

    bool result = true;

    // There are two configuration options which are mandatory when using the Google Sheets integration
    if (strlen(config.scriptsURL) <= GSCRIPTS_MIN_URL_LENGTH ||
        strlen(config.scriptsEmail) < GSCRIPTS_MIN_EMAIL_LENGTH)
    {
        // Log.verbose(D("Either scriptsURL or scriptsEmail not populated. Returning." CR));
        return false;
    }

    // This should look like this when sent to the proxy that sends to Google (once per Tilt):
    // {
    //   'payload': {     // Payload is what gets ultimately sent on to Google Scripts
    //        'Beer':     'Key Goes Here',
    //        'Temp':     65,
    //        'SG':       1.050,  // This is sent as a float
    //        'Color':    'Blue',
    //        'Comment':  '',
    //        'Email':    'xxx@gmail.com',
    //    },
    //   'gscripts_url': 'https://script.google.com/.../',  // This is specific to the proxy
    // }
    //
    // For secure GScripts support, we don't send the 'j' json object - just the payload.

    // Loop through each of the tilt colors cached by tilt_scanner, sending data for each of the active tilts
    for (uint8_t i = 0; i < TILT_COLORS; i++)
    {
        if (tilt_scanner.tilt(i)->is_loaded())
        {
            if (tilt_scanner.tilt(i)->gsheets_beer_name().length() <= 0)
            {
                continue; // If there is no gsheets beer name, we don't know where to log to. Skip this tilt.
            }

            payload["Beer"] = tilt_scanner.tilt(i)->gsheets_beer_name();
            payload["Temp"] = tilt_scanner.tilt(i)->converted_temp(true); // Always in Fahrenheit
            payload["SG"] = tilt_scanner.tilt(i)->converted_gravity(false);
            payload["Color"] = tilt_scanner.tilt(i)->color_name();
            payload["Comment"] = "";
            payload["Email"] = config.scriptsEmail; // The gmail email address associated with the script on google
            payload["tzOffset"] = config.TZoffset;

            char payload_string[500];
            serializeJson(payload, payload_string);
            // When sending the data to GScripts directly, we're sending the payload - not the wrapped payload
            if (!send_to_url_https(config.scriptsURL, "", payload_string, "application/json"))
                result = false; // There was an error with the previous send
        }
    }
    return result;
}

bool dataSendHandler::send_to_bf_and_bf(const uint8_t which_bf)
{
    // TODO: (JSON) Come back and tighten this up

    // This function combines the data formatting for both "BF"s - Brewers Friend & Brewfather
    // Once the data is formatted, it is dispatched to send_to_url to be sent out.

    bool result = true;
    StaticJsonDocument<500> j;
    char apiKey[26];
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

    // The data should look like this when sent to Brewfather or Brewers Friend (once per Tilt):
    // {
    //   'name':            'Red',  // The color of the Tilt
    //   'temp':            65,
    //   'temp_unit':       'F',    // Always in Fahrenheit
    //   'gravity':         1.050,  // This is sent as a float
    //   'gravity_unit':    'G',    // We send specific gravity, not plato
    //   'device_source':   'TiltBridge',
    // }

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

bool dataSendHandler::send_to_url(const char *url, const char *apiKey, const char *dataToSend, const char *contentType, bool checkBody, const char* bodyCheck)
{
    // This handles the generic act of sending data to an endpoint
    bool retVal = false;

    if (strlen(dataToSend) > 5 && strlen(url) > 8)
    {
        LCBUrl lcburl;
        lcburl.setUrl(url);

        bool validTarget = false;
        if (lcburl.isMDNS())
        {
            // Make sure we can resolve the address
            if (lcburl.getIP() != INADDR_NONE)
                validTarget = true;
        }
        else if (lcburl.getHost() == lcburl.getIP().toString())
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
            if (lcburl.isMDNS())
                // Use the IP address we resolved (necessary for mDNS)
                Log.notice(F("Connecting to: %s at %s on port %l" CR),
                            lcburl.getHost().c_str(),
                            lcburl.getIP().toString().c_str(),
                            lcburl.getPort());
            else
                Log.notice(F("Connecting to: %s on port %l" CR),
                            lcburl.getHost().c_str(),
                            lcburl.getPort());

            WiFiClient client;
            //  1 = SUCCESS
            //  0 = FAILED
            // -1 = TIMED_OUT
            // -2 = INVALID_SERVER
            // -3 = TRUNCATED
            // -4 = INVALID_RESPONSE
            client.setNoDelay(true);
            client.setTimeout(10000);
            if (client.connect(lcburl.getIP(), lcburl.getPort()))
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
                client.print(F("POST /"));
                client.print(lcburl.getPath().c_str());
                if (lcburl.getAfterPath().length() > 0)
                {
                    client.print(lcburl.getAfterPath().c_str());
                }
                client.println(F(" HTTP/1.1"));

                // Begin headers
                //
                // Host
                Log.verbose(F("Host: %s:%l" CR), lcburl.getHost().c_str(), lcburl.getPort());
                client.print(F("Host: "));
                client.print(lcburl.getHost().c_str());
                client.print(F(":"));
                client.println(lcburl.getPort());
                //
                Log.verbose(F("Connection: close" CR));
                client.println(F("Connection: close"));
                // Content
                Log.verbose(F("Content-Length: %l" CR), strlen(dataToSend));
                client.print(F("Content-Length: "));
                client.println(strlen(dataToSend));
                // Content Type
                Log.verbose(F("Content-Type: %s" CR), contentType);
                client.print(F("Content-Type: "));
                client.println(contentType);
                // API Key
                if (strlen(apiKey) > 2)
                {
                        Log.verbose(F("X-API-KEY: %s" CR), apiKey);
                        client.print(F("X-API-KEY: "));
                        client.println(apiKey);
                }
                // Terminate headers with a blank line
                Log.verbose(F("End headers." CR));
                client.println();
                //
                // End Headers

                // Post JSON
                client.println(dataToSend);
                // Check the HTTP status (should be "HTTP/1.1 200 OK")
                char status[32] = {0};
                client.readBytesUntil('\r', status, sizeof(status));
                client.stop();
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
                            lcburl.getHost().c_str(), lcburl.getPort(), client.connected());
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
    return result;
}

#ifdef ENABLE_TEST_CHECKINS
u_long checkin_no = 0;

void send_checkin_stat()
{
    HTTPClient http;

    String Data = "checkin_no=";
    Data += String(checkin_no);
    //    Data += "\r\n\r\n";

    //    Serial.print("Data to send: ");
    //    Serial.println(Data);

    http.begin("http://www.fermentrack.com/checkin/");  //Specify destination for HTTP request
    http.addHeader("Content-Type", "application/json"); //Specify content-type header
    int httpResponseCode = http.POST(Data);             //Send the actual POST request

    if (httpResponseCode > 0)
    {
        //        String response = http.getString();                       //Get the response to the request
        //        Serial.println(httpResponseCode);   //Print return code
        //        Serial.println(response);           //Print request answer
    }
    else
    {
        //        Serial.print("Error on sending POST: ");
        //        Serial.println(httpResponseCode);
    }
    http.end(); //Free resources

    checkin_no = checkin_no + 1;
}
#endif

void dataSendHandler::process()
{
    // dataSendHandler::process() processes each tick & dispatches HTTP clients to push data out as necessary

    // Check & send to Local Target if necessary
    if (send_to_localTarget_at <= xTaskGetTickCount())
    {
        if (WiFiClass::status() == WL_CONNECTED && strlen(config.localTargetURL) > LOCALTARGET_MIN_URL_LENGTH)
        { //Check WiFi connection status
            Log.verbose(F("Calling send to Local Target." CR));

            send_to_localTarget();
            send_to_localTarget_at = xTaskGetTickCount() + (config.localTargetPushEvery * 1000);
        }
        else
        {
            // If the user adds the setting, we want this to kick in within 10 seconds
            send_to_localTarget_at = xTaskGetTickCount() + 10000;
        }
        yield();
    }

    // Check & send to Brewstatus if necessary
    if (send_to_brewstatus_at <= xTaskGetTickCount())
    {
        if (WiFiClass::status() == WL_CONNECTED && strlen(config.brewstatusURL) > BREWSTATUS_MIN_URL_LENGTH)
        { //Check WiFi connection status
            Log.verbose(F("Calling send to Brewstatus." CR));

            send_to_brewstatus();
            send_to_brewstatus_at = xTaskGetTickCount() + (config.brewstatusPushEvery * 1000);
        }
        else
        {
            // If the user adds the setting, we want this to kick in within 10 seconds
            send_to_brewstatus_at = xTaskGetTickCount() + 10000;
        }
        yield();
    }

    // Check & send to Google Scripts if necessary
    if (send_to_google_at <= xTaskGetTickCount())
    {
        if (WiFiClass::status() == WL_CONNECTED && strlen(config.scriptsURL) > GSCRIPTS_MIN_URL_LENGTH)
        {
            Log.verbose(F("Calling send to Google." CR));
            // tilt_scanner.wait_until_scan_complete();
            send_to_google();
            send_to_google_at = xTaskGetTickCount() + GSCRIPTS_DELAY;
        }
        else
        {
            // If the user adds the setting, we want this to kick in within 10 seconds
            send_to_google_at = xTaskGetTickCount() + 10000;
        }
        yield();
    }

    // Check & send to Brewers Friend if necessary
    if (send_to_brewers_friend_at <= xTaskGetTickCount())
    {
        if (WiFiClass::status() == WL_CONNECTED && strlen(config.brewersFriendKey) > BREWERS_FRIEND_MIN_KEY_LENGTH)
        {
            Log.verbose(F("Calling send to Brewers Friend." CR));

            send_to_bf_and_bf(BF_MEANS_BREWERS_FRIEND);
            send_to_brewers_friend_at = xTaskGetTickCount() + BREWERS_FRIEND_DELAY;
        }
        else
        {
            // If the user adds the setting, we want this to kick in within 10 seconds
            send_to_brewers_friend_at = xTaskGetTickCount() + 10000;
        }
        yield();
    }

#ifdef ENABLE_TEST_CHECKINS
    if (send_checkin_at <= xTaskGetTickCount())
    {
        if (WiFiClass::status() == WL_CONNECTED)
        { //Check WiFi connection status
            Log.verbose(F("Calling check-in to fermentrack.com." CR));
            // tilt_scanner.wait_until_scan_complete();
            send_checkin_stat();
        }
        send_checkin_at = xTaskGetTickCount() + (60 * 5 * 1000);
        yield();
    }
#endif

    if (send_to_brewfather_at <= xTaskGetTickCount())
    {
        if (WiFiClass::status() == WL_CONNECTED && strlen(config.brewfatherKey) > BREWFATHER_MIN_KEY_LENGTH)
        {
            Log.verbose(F("Calling send to Brewfather." CR));

            send_to_bf_and_bf(BF_MEANS_BREWFATHER);
            send_to_brewfather_at = xTaskGetTickCount() + BREWFATHER_DELAY;
        }
        else
        {
            // If the user adds the setting, we want this to kick in within 10 seconds
            send_to_brewfather_at = xTaskGetTickCount() + 10000;
        }
        yield();
    }

    // Check & send to mqtt broker if necessary
    if (send_to_mqtt_at <= xTaskGetTickCount())
    {
        //if (WiFiClass::status() == WL_CONNECTED && strlen(config.mqttBrokerHost) > IP_MIN_STRING_LENGTH) // TODO:  Replace this check
        if (true)
        { //Check WiFi connection status
            Log.verbose(F("Publishing available results to MQTT Broker." CR));

            send_to_mqtt();
            send_to_mqtt_at = xTaskGetTickCount() + (config.mqttPushEvery * 1000);
        }
        else
        {
            // If the user adds the setting, we want this to kick in within 10 seconds
            send_to_mqtt_at = xTaskGetTickCount() + 10000;
        }
        yield();
    }
}
