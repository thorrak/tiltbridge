#include "sendData.h"

dataSendHandler data_sender; // Global data sender

MQTTClient mqttClient(256);

// POST Timers
Ticker cloudTargetTicker;
Ticker localTargetTicker;
Ticker brewersFriendTicker;
Ticker brewfatherTicker;
Ticker grainfatherTicker;
Ticker brewStatusTicker;
Ticker taplistioTicker;
Ticker gSheetsTicker;
Ticker mqttTicker;

// POST Semaphores
bool send_cloudTarget = false;
bool send_localTarget = false;
bool send_brewersFriend = false;
bool send_brewfather = false;
bool send_grainfather = false;
bool send_brewStatus = false;
bool send_taplistio = false;
bool send_gSheets = false;
bool send_mqtt = false;
bool send_lock = false;

dataSendHandler::dataSendHandler() {}

void dataSendHandler::init()
{
    init_mqtt();

    // Set up timers
    // DEBUG:
    cloudTargetTicker.once(10, [](){send_cloudTarget = true;});      // Schedule first send to Cloud Target
    localTargetTicker.once(20, [](){send_localTarget = true;});      // Schedule first send to Local Target
//    localTargetTicker.once(5, [](){send_localTarget = true;});      // Schedule first send to Local Target
    // DEBUG^
    brewStatusTicker.once(30, [](){send_brewStatus = true;});        // Schedule first send to Brew Status
    brewfatherTicker.once(40, [](){send_brewfather = true;});        // Schedule first send to Brewfather
    brewersFriendTicker.once(50, [](){send_brewersFriend = true;});  // Schedule first send to Brewer's Friend
    mqttTicker.once(60, [](){send_mqtt = true;});                    // Schedule first send to MQTT
    gSheetsTicker.once(70, [](){send_gSheets = true;});              // Schedule first send to Google Sheets
    grainfatherTicker.once(80, [](){send_grainfather = true;});      // Schedule first send to Grainfather
    taplistioTicker.once(80, [](){send_taplistio = true;});          // Schedule first send to Taplist.io
}

bool dataSendHandler::send_to_localTarget()
{
    bool result = true;

    if (send_localTarget && ! send_lock)
    {
        // Local Target
        send_localTarget = false;
        send_lock = true;
//        tilt_scanner.deinit();

        if (WiFiClass::status() == WL_CONNECTED && strlen(config.localTargetURL) >= LOCALTARGET_MIN_URL_LENGTH)
        {
            Log.verbose(F("Calling send to Local Target.\r\n"));
            DynamicJsonDocument doc(TILT_ALL_DATA_SIZE + 128);
            char tilt_data[TILT_ALL_DATA_SIZE + 128];

            tilt_scanner.tilt_to_json_string(tilt_data, true);

            doc["mdns_id"] = config.mdnsID;
            doc["tilts"] = serialized(tilt_data);

            serializeJson(doc, tilt_data);

            if (send_to_url(config.localTargetURL, "", tilt_data, "application/json"))
            {
                Log.notice(F("Completed send to Local Target.\r\n"));
            }
            else
            {
                result = false; // There was an error with the previous send
                Log.verbose(F("Error sending to Local Target.\r\n"));
            }
        }
        localTargetTicker.once(config.localTargetPushEvery, [](){send_localTarget = true;}); // Set up subsequent send to localTarget
//        tilt_scanner.init();
        send_lock = false;
    }
    return result;
}

bool send_to_bf_and_bf()
{
    bool retval = false;
    if (send_brewersFriend && ! send_lock)
    {
        send_lock = true;
        // Brewer's Friend
        send_brewersFriend = false;
        if (WiFiClass::status() == WL_CONNECTED && strlen(config.brewersFriendKey) > BREWERS_FRIEND_MIN_KEY_LENGTH)
        {
            Log.verbose(F("Calling send to Brewer's Friend.\r\n"));
            retval = data_sender.send_to_bf_and_bf(BF_MEANS_BREWERS_FRIEND);
            if (retval)
            {
                Log.notice(F("Completed send to Brewer's Friend.\r\n"));
            }
            else
            {
                Log.verbose(F("Error sending to Brewer's Friend.\r\n"));
            }
        }
        brewersFriendTicker.once(BREWERS_FRIEND_DELAY, [](){send_brewersFriend = true;}); // Set up subsequent send to Brewer's Friend
        send_lock = false;
    }

    if (send_brewfather && ! send_lock)
    {
        send_lock = true;
        // Brewfather
        send_brewfather = false;
        if (WiFiClass::status() == WL_CONNECTED && strlen(config.brewfatherKey) > BREWFATHER_MIN_KEY_LENGTH)
        {
            Log.verbose(F("Calling send to Brewfather.\r\n"));
            retval = data_sender.send_to_bf_and_bf(BF_MEANS_BREWFATHER);
            if (retval)
            {
                Log.notice(F("Completed send to Brewfather.\r\n"));
            }
            else
            {
                Log.verbose(F("Error sending to Brewfather.\r\n"));
            }
        }
        brewfatherTicker.once(BREWFATHER_DELAY, [](){send_brewfather = true;}); // Set up subsequent send to Brewfather
        send_lock = false;
    }
    return retval;
}

void send_to_cloud()
{
    if (send_cloudTarget && ! send_lock) {
        send_lock = true;
        send_cloudTarget = false;
        addTiltToParse();
        cloudTargetTicker.once(CLOUD_DELAY, [](){send_cloudTarget = true;}); // Set up subsequent send to localTarget
    }
    send_lock = false;
}

bool dataSendHandler::send_to_bf_and_bf(const uint8_t which_bf)
{
    // This function combines the data formatting for both "BF"s - Brewers
    // Friend & Brewfather. Once the data is formatted, it is dispatched
    // to send_to_url to be sent out.

    bool result = true;
    StaticJsonDocument<BF_SIZE> j;
    char url[128];

    // As this function is being used for both Brewer's Friend and Brewfather,
    // let's determine which we want and set up the URL/API key accordingly.
    if (which_bf == BF_MEANS_BREWFATHER)
    {
        if (strlen(config.brewfatherKey) <= BREWFATHER_MIN_KEY_LENGTH)
        {
            Log.verbose(F("Brewfather key not populated. Returning.\r\n"));
            return false;
        }
        strcpy(url, "http://log.brewfather.net/stream?id=");
        strcat(url, config.brewfatherKey);
    }
    else if (which_bf == BF_MEANS_BREWERS_FRIEND)
    {
        if (strlen(config.brewersFriendKey) <= BREWERS_FRIEND_MIN_KEY_LENGTH)
        {
            Log.verbose(F("Brewer's Friend key not populated. Returning.\r\n"));
            return false;
        }
        strcpy(url, "http://log.brewersfriend.com/stream/");
        strcat(url, config.brewersFriendKey);
    }
    else
    {
        Log.error(F("Invalid value of which_bf passed to send_to_bf_and_bf.\r\n"));
        return false;
    }

    // Loop through each of the tilt colors cached by tilt_scanner, sending
    // data for each of the active tilts
    for (uint8_t i = 0; i < TILT_COLORS; i++)
    {
        if (tilt_scanner.tilt(i)->is_loaded())
        {
            Log.verbose(F("Tilt loaded with color name: %s\r\n"), tilt_color_names[i]);
            j["name"] = tilt_color_names[i];
            j["temp"] = tilt_scanner.tilt(i)->converted_temp(true); // Always in Fahrenheit
            j["temp_unit"] = "F";
            j["gravity"] = tilt_scanner.tilt(i)->converted_gravity(false);
            j["gravity_unit"] = "G";
            j["device_source"] = "TiltBridge";

            char payload_string[BF_SIZE];
            serializeJson(j, payload_string);

            if (!send_to_url(url, "", payload_string, "application/json"))
                result = false; // There was an error with the previous send
        }
    }
    return result;
}

bool dataSendHandler::send_to_grainfather()
{
    bool result = true;
    StaticJsonDocument<GF_SIZE> j;

    if (send_grainfather && ! send_lock)
    {
        // Brew Status
        send_grainfather = false;
        send_lock = true;
        if (WiFiClass::status() == WL_CONNECTED)
        {
            Log.verbose(F("Calling send to Grainfather.\r\n"));

            // Loop through each of the tilt colors cached by tilt_scanner, sending
            // data for each of the active tilts
            for (uint8_t i = 0; i < TILT_COLORS; i++)
            {
                if (strlen(config.grainfatherURL[i].link) == 0) {
                    continue;
                }

                if (tilt_scanner.tilt(i)->is_loaded())
                {
                    Log.verbose(F("Tilt loaded with color name: %s\r\n"), tilt_color_names[i]);
                    j["Temp"] = tilt_scanner.tilt(i)->converted_temp(true); // Always in Fahrenheit
                    j["Unit"] = "F";
                    j["SG"] = tilt_scanner.tilt(i)->converted_gravity(false);

                    char payload_string[GF_SIZE];
                    serializeJson(j, payload_string);

                    if (!http_send_json(config.grainfatherURL[i].link, payload_string))
                    {
                        result = false; // There was an error with the previous send
                    }
                }
            }
        }
        grainfatherTicker.once(GRAINFATHER_DELAY, [](){send_grainfather = true;}); // Set up subsequent send to Grainfather
        send_lock = false;
    }
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
        if (WiFiClass::status() == WL_CONNECTED && strlen(config.brewstatusURL) > BREWSTATUS_MIN_URL_LENGTH)
        {
            Log.verbose(F("Calling send to Brew Status.\r\n"));

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
                            tilt_color_names[i],
                            ((double)std::time(0) + (config.TZoffset * 3600.0)) / 86400.0 + 25569.0);
                    if (send_to_url(config.brewstatusURL, "", payload, "application/x-www-form-urlencoded"))
                    {
                        Log.notice(F("Completed send to Brew Status.\r\n"));
                    }
                    else
                    {
                        result = false;
                        Log.verbose(F("Error sending to Brew Status.\r\n"));
                    }
                }
            }
        }
        brewStatusTicker.once(config.brewstatusPushEvery, [](){send_brewStatus = true;}); // Set up subsequent send to Brew Status
        send_lock = false;
    }
    return result;
}

bool dataSendHandler::http_send_json(const char * url, const char * payload)
{
    int httpResponseCode;
    StaticJsonDocument<GF_SIZE> retval;
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.setConnectTimeout(6000);
    http.setReuse(false);

    secureClient.setInsecure();

    http.addHeader(F("Content-Type"), F("application/json"));
    http.addHeader(F("Accept"), F("application/json"));
    httpResponseCode = http.POST(payload);

    if (httpResponseCode >= 400) {
        Log.error(F("HTTP error %d: %s, %s.\r\n"), httpResponseCode, http.errorToString(httpResponseCode).c_str(), http.getString().c_str());
        return false;
    }

    if (!http.begin(secureClient, url)) {
        Log.error(F("Unable to create secure connection to %s.\r\n"), url);
        return false;
    }

    deserializeJson(retval, http.getString().c_str());

    http.end();
    retval.clear();

    return true;
}

bool dataSendHandler::send_to_taplistio()
{
    bool result = true;

    // See if it's our time to send.
    if (!send_taplistio) {
        return false;
    } else if (send_lock) {
        return false;
    }

    // Attempt to send.
    send_taplistio = false;
    send_lock = true;

    if (WiFiClass::status() != WL_CONNECTED) {
        Log.verbose(F("taplist.io: Wifi not connected, skipping send.\r\n"));
        taplistioTicker.once(config.taplistioPushEvery, [](){send_taplistio = true;});
        send_lock = false;
        return false;
    }

    char userAgent[128];
    snprintf(userAgent, sizeof(userAgent),
        "tiltbridge/%s (branch %s; build %s)",
        version(),
        branch(),
        build()
    );

    for (uint8_t i = 0; i < TILT_COLORS; i++) {
        StaticJsonDocument<192> j;
        char payload_string[192];
        int httpResponseCode;

        if (!tilt_scanner.tilt(i)->is_loaded()) {
            continue;
        }

        j["Color"] = tilt_color_names[i];
        j["Temp"] = tilt_scanner.tilt(i)->converted_temp(false);
        j["SG"] = tilt_scanner.tilt(i)->converted_gravity(false);
        j["temperature_unit"] = "F";
        j["gravity_unit"] = "G";

        serializeJson(j, payload_string);

        http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
        http.setConnectTimeout(6000);
        http.setReuse(false);
        secureClient.setInsecure();

        Log.verbose(F("taplist.io: Sending %s Tilt to %s\r\n"), tilt_color_names[i], config.taplistioURL);

        if (!http.begin(secureClient, config.taplistioURL)) {
            Log.error(F("taplist.io: Unable to create secure connection to %s\r\n"), config.taplistioURL);
            result = false;
            break;
        }

        http.addHeader(F("Content-Type"), F("application/json"));
        http.setUserAgent(userAgent);
        httpResponseCode = http.POST(payload_string);

        if (httpResponseCode < HTTP_CODE_OK || httpResponseCode > HTTP_CODE_NO_CONTENT) {
            Log.error(F("taplist.io: send %s Tilt failed (%d): %s. Response:\r\n%s\r\n"),
                tilt_color_names[i],
                httpResponseCode,
                http.errorToString(httpResponseCode).c_str(),
                http.getString().c_str());
            result = false;
        } else {
            Log.verbose(F("taplist.io: %s Tilt: success!\r\n"), tilt_color_names[i]);
        }
    }

    taplistioTicker.once(config.taplistioPushEvery, [](){send_taplistio = true;});
    send_lock = false;
    return result;
}

bool dataSendHandler::send_to_google()
{
    bool result = true;

    if (send_gSheets && !send_lock) {
        // Google Sheets
        send_gSheets = false;
        send_lock = true;

        //tilt_scanner.deinit();
        StaticJsonDocument<GSHEETS_JSON> payload;
        char payload_string[GSHEETS_JSON];
        StaticJsonDocument<GSHEETS_JSON> retval;
        int httpResponseCode;
        int numSent = 0;
#if (ARDUINO_LOG_LEVEL == 6)
        char buff[1024] = "";
#endif

        // The google sheets handler only fires if we have both a Google Scripts URL to post to, and an email address.
        if (strlen(config.scriptsURL) >= GSCRIPTS_MIN_URL_LENGTH && strlen(config.scriptsEmail) >= GSCRIPTS_MIN_EMAIL_LENGTH) {
            Log.verbose(F("Checking for any pending Google Sheets pushes.\r\n"));
//            Log.verbose(F("Executing on core %i.\r\n"), xPortGetCoreID());
            printMem();

            for (uint8_t i = 0; i < TILT_COLORS; i++) {
                // Loop through each of the tilt colors and check if it is both available and has active data
                if (tilt_scanner.tilt(i)->is_loaded()) {
                    // Check if there is a google sheet name associated with the specific Tilt
                    if (strlen(config.gsheets_config[i].name) > 0) {
                        // If there's a sheet name saved, then we should send the data
                        if (numSent == 0)
                            Log.notice(F("Beginning GSheets check-in.\r\n"));
                        payload["Beer"] = config.gsheets_config[i].name;
                        payload["Temp"] = tilt_scanner.tilt(i)->converted_temp(true); // Always send in Fahrenheit
                        payload["SG"] = tilt_scanner.tilt(i)->converted_gravity(false);
                        payload["Color"] = tilt_color_names[i];
                        payload["Comment"] = "";
                        payload["Email"] = config.scriptsEmail; // The gmail email address associated with the script on google
                        payload["tzOffset"] = config.TZoffset;

                        serializeJson(payload, payload_string);
                        payload.clear();

                        http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);  // Follow the 301
                        http.setConnectTimeout(6000);                           // Set 6 second timeout
                        http.setReuse(false);
                        secureClient.setInsecure();                             // Ignore SHA fingerprint

                        if (!http.begin(secureClient, config.scriptsURL)) {      // Connect secure
                            Log.error(F("Unable to create secure connection to %s.\r\n"), config.scriptsURL);
                            result = false;
                        } else {
                            // Failed to open a connection
                            Log.verbose(F("Created secure connection to %s.\r\n"), config.scriptsURL);
                            Log.verbose(F("Sending the following payload to Google Sheets (%s):\r\n\t\t%s\r\n"), tilt_color_names[i], payload_string);

                            http.addHeader(F("Content-Type"), F("application/json"));   // Specify content-type header
                            httpResponseCode = http.POST(payload_string);               // Send the payload

                            if (httpResponseCode == HTTP_CODE_OK) {  // HTTP_CODE_OK = 200
                                // POST success
#if (ARDUINO_LOG_LEVEL == 6)
                                // We need to use a buffer in order to be able to use the response twice
                                strlcpy(buff, http.getString().c_str(), 1024);
                                Log.verbose(F("HTTP Response: 200\r\nFull Response:\r\n\t%s\r\n"), buff);
                                deserializeJson(retval, buff);
//                                deserializeJson(retval, http.getString().c_str());
#else
                                deserializeJson(retval, http.getString().c_str());
#endif

                                if(strcmp(config.gsheets_config[i].link, retval["doclongurl"].as<String>().c_str()) != 0) {
                                    Log.verbose(F("Storing new doclongurl: %s.\r\n"), retval["doclongurl"].as<String>().c_str());
                                    strlcpy(config.gsheets_config[i].link, retval["doclongurl"].as<String>().c_str(), 255);
                                    config.save();
                                }
                                retval.clear();
                                numSent++;
                            } else {
                                // Post generated an error (response code != 200)
                                Log.error(F("Google send to %s Tilt failed (%d): %s. Response:\r\n%s\r\n"),
                                    tilt_color_names[i],
                                    httpResponseCode,
                                    http.errorToString(httpResponseCode).c_str(),
                                    http.getString().c_str());
                                result = false;
                            } // Response code != 200
                        } // Good connection
                        http.end();
                        delay(100);  // Give garbage collection time to run
                    } // Check we have a sheet name for the color
                } // Check scanner is loaded for color
            } // Loop through colors
            Log.notice(F("Submitted %l sheet%s to Google.\r\n"), numSent, (numSent== 1) ? "" : "s");

        }
        gSheetsTicker.once(GSCRIPTS_DELAY, [](){send_gSheets = true;}); // Set up subsequent send to Google Sheets

        //tilt_scanner.init();
        send_lock = false;
    }
    return result;
}

void dataSendHandler::init_mqtt()
{
    LCBUrl url;

    if (strcmp(config.mqttBrokerHost, "") != 0 || strlen(config.mqttBrokerHost) != 0)
    {
        if (url.isMDNS(config.mqttBrokerHost)) {
            Log.verbose(F("Initializing connection to MQTTBroker: %s (%s) on port: %d\r\n"),
                config.mqttBrokerHost,
                url.getIP(config.mqttBrokerHost).toString().c_str(),
                config.mqttBrokerPort);
        } else {
            Log.verbose(F("Initializing connection to MQTTBroker: %s on port: %d\r\n"),
                config.mqttBrokerHost,
                config.mqttBrokerPort);
        }

        if (mqtt_alreadyinit) {
            mqttClient.disconnect();
            delay(250);
            if (url.isMDNS(config.mqttBrokerHost)) {
                mqttClient.setHost(url.getIP(config.mqttBrokerHost), config.mqttBrokerPort);
            } else {
                mqttClient.setHost(config.mqttBrokerHost, config.mqttBrokerPort);
            }
        } else {
            if (url.isMDNS(config.mqttBrokerHost)) {
                mqttClient.begin(url.getIP(config.mqttBrokerHost), config.mqttBrokerPort, mqClient);
            } else {
                mqttClient.begin(config.mqttBrokerHost, config.mqttBrokerPort, mqClient);
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
                Log.verbose(F("Connecting to: %s at %s on port %l\r\n"),
                            lcburl.getHost().c_str(),
                            lcburl.getIP(lcburl.getHost().c_str() ).toString().c_str(),
                            lcburl.getPort());
            else
                Log.verbose(F("Connecting to: %s on port %l\r\n"),
                            lcburl.getHost().c_str(),
                            lcburl.getPort());

            if (client.connect(lcburl.getIP(lcburl.getHost().c_str()), lcburl.getPort()))
            {
                Log.verbose(F("Connected to: %s.\r\n"), lcburl.getHost().c_str());

                // Open POST connection
                if (lcburl.getAfterPath().length() > 0)
                {
                    Log.verbose(F("POST /%s%s HTTP/1.1\r\n"),
                                lcburl.getPath().c_str(),
                                lcburl.getAfterPath().c_str());
                }
                else
                {
                    Log.verbose(F("POST /%s HTTP/1.1\r\n"), lcburl.getPath().c_str());
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
                Log.verbose(F("Host: %s:%l\r\n"), lcburl.getHost().c_str(), lcburl.getPort());
                client.print(F("Host: "));
                client.print(lcburl.getHost().c_str());
                client.print(F(":"));
                client.println(lcburl.getPort());
                //
                Log.verbose(F("Connection: close\r\n"));
                client.println(F("Connection: close"));
                // Content
                Log.verbose(F("Content-Length: %l\r\n"), strlen(dataToSend));
                client.print(F("Content-Length: "));
                client.println(strlen(dataToSend));
                // Content Type
                Log.verbose(F("Content-Type: %s\r\n"), contentType);
                client.print(F("Content-Type: "));
                client.println(contentType);
                // API Key
                if (strlen(apiKey) > 2)
                {
                        Log.verbose(F("X-API-KEY: %s\r\n"), apiKey);
                        client.print(F("X-API-KEY: "));
                        client.println(apiKey);
                }
                // Terminate headers with a blank line
                Log.verbose(F("End headers.\r\n"));
                client.println();
                //
                // End Headers

                // Post JSON
                client.println(dataToSend);
                // Check the HTTP status (should be "HTTP/1.1 200 OK")
                char status[32] = {0};
                client.readBytesUntil('\r', status, sizeof(status));
                client.stop();
                Log.verbose(F("Status: %s\r\n"), status);
                if (strcmp(status + 9, "200 OK") == 0)
                {
                    if (checkBody == true) // We can do additional checks here
                    {
                        // Check body
                        String response = String(status);
                        if (response.indexOf(bodyCheck) >= 0)
                        {
                            Log.verbose(F("Response body ok.\r\n"));
                            retVal = true;
                        }
                        else
                        {
                            Log.error(F("Unexpected body content: %s\r\n"), response.c_str());
                            retVal = false;
                        }
                    }
                    else
                    {
                        Log.verbose(F("Post to %s was successful.\r\n"), lcburl.getHost().c_str());
                        retVal = true;
                    }
                }
                else
                {
                    Log.error(F("Unexpected status: %s\r\n"), status);
                    retVal = false;
                }
            }
            else
            {
                Log.warning(F("Connection failed, Host: %s, Port: %l (Err: %d)\r\n"),
                            lcburl.getHost().c_str(), lcburl.getPort(), client.connected());
                retVal = false;
            }
        }
        else
        {
            Log.error(F("Invalid target: %s.\r\n"), url);
        }
    }
    else
    {
        Log.notice(F("No URL provided, or no data to send.\r\n"));
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

    if (send_mqtt && ! send_lock)
    {
        // MQTT
        send_mqtt = false;
        send_lock = true;
        if (strcmp(config.mqttBrokerHost, "") != 0 || strlen(config.mqttBrokerHost) != 0)
        {
            Log.verbose(F("Publishing available results to MQTT Broker.\r\n"));
            // Function sends three payloads with the first two designed to
            // support autodiscovery and configuration on Home Assistant.
            // General payload formatted as json when sent to mqTT:
            //{"Color":"Black","SG":"1.0180","Temp":"73.0","fermunits":"SG","tempunits":"F","timeStamp":1608745710}
            //
            // Loop through each of the tilt colors cached by tilt_scanner,
            // sending data for each of the active tilts
            for (uint8_t i = 0; i < TILT_COLORS; i++)
            {
                if (tilt_scanner.tilt(i)->is_loaded())
                {
                    char tilt_topic[50] = {'\0'};
                    snprintf(tilt_topic, 50, "%s/tilt_%s",
                            config.mqttTopic,
                            tilt_color_names[i]);

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
                                    tilt_color_names[i]);
                            payload["dev_cla"] = "temperature";
                            strcat(unit, "\u00b0");
                            strcat(unit, config.tempUnit);
                            payload["unit_of_meas"] = unit;
                            payload["ic"] = "mdi:thermometer";
                            payload["stat_t"] = tilt_topic;
                            strcat(tilt_name, "Tilt Temperature - ");
                            strcat(tilt_name, tilt_color_names[i]);
                            payload["name"] = tilt_name;
                            payload["val_tpl"] = "{{value_json.Temp}}";
                            retain = true;
                            break;
                        case 1: //Home Assistant Config Topic for Sp Gravity
                            sprintf(m_topic, "homeassistant/sensor/%s_tilt_%sG/config",
                                    config.mqttTopic,
                                    tilt_color_names[i]);
                            //payload["dev_cla"] = "None";
                            payload["unit_of_meas"] = "SG";
                            //payload["ic"] = "";
                            payload["stat_t"] = tilt_topic;
                            strcat(tilt_name, "Tilt Specific Gravity - ");
                            strcat(tilt_name, tilt_color_names[i]);
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
                            payload["Color"] = tilt_color_names[i];
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

                        Log.verbose(F("Topic: %s\r\n"), m_topic);
                        Log.verbose(F("Message: %s\r\n"), payload_string);

                        if (!mqttClient.connected() && j == 0)
                        {
                            Log.warning(F("MQTT disconnected. Attempting to reconnect to MQTT Broker\r\n"));
                            connect_mqtt();
                        }

                        result = mqttClient.publish(m_topic, payload_string, retain, 0);
                        delay(10);

                        payload.clear();
                    }
                }
            }
            if (result) {
                Log.notice(F("Completed publish to MQTT Broker.\r\n"));
            } else {
                result = false; // There was an error with the previous send
                Log.verbose(F("Error publishing to MQTT Broker.\r\n"));
            }
        }
        mqttTicker.once(config.mqttPushEvery, [](){send_mqtt = true;});   // Set up subsequent send to MQTT
        send_lock = false;
    }

    return result;
}
