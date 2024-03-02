#include <ctime>
#include <ArduinoJson.h>
#include <Ticker.h>

#include <WiFi.h>
#include <MQTT.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include <ArduinoLog.h>
#include <LCBUrl.h>

#include "tilt/tiltScanner.h"
#include "jsonconfig.h"
#include "version.h"
#include "parseTarget.h"
#include "http_server.h"
#include "main.h"  // for printMem()

#include "sendData.h"


dataSendHandler data_sender; // Global data sender

MQTTClient mqttClient(512);

dataSendHandler::dataSendHandler() {}

void dataSendHandler::init()
{
    init_mqtt();

    // Set up timers
//    localTargetTicker.once(5, [](){data_sender.send_localTarget = true;});      // Schedule first send to Local Target
    localTargetTicker.once(10, [](){data_sender.send_localTarget = true;});      // Schedule first send to Local Target
    mqttTicker.once(20, [](){data_sender.send_mqtt = true;});                    // Schedule first send to MQTT
    brewStatusTicker.once(30, [](){data_sender.send_brewStatus = true;});        // Schedule first send to Brew Status
    brewfatherTicker.once(40, [](){data_sender.send_brewfather = true;});        // Schedule first send to Brewfather
    brewersFriendTicker.once(50, [](){data_sender.send_brewersFriend = true;});  // Schedule first send to Brewer's Friend
    userTargetTicker.once(60, [](){data_sender.send_userTarget = true;});        // Schedule first send to User-defined JSON target
    gSheetsTicker.once(70, [](){data_sender.send_gSheets = true;});              // Schedule first send to Google Sheets
    grainfatherTicker.once(80, [](){data_sender.send_grainfather = true;});      // Schedule first send to Grainfather
    taplistioTicker.once(90, [](){data_sender.send_taplistio = true;});          // Schedule first send to Taplist.io
    cloudTargetTicker.once(100, [](){data_sender.send_cloudTarget = true;});     // Schedule first send to Cloud Target
}

void dataSendHandler::process()
{
    if (WiFiClass::status() == WL_CONNECTED) {
        send_to_cloud();
        send_to_localTarget();
        send_to_bf_and_bf();
        send_to_grainfather();
        send_to_brewstatus();
        send_to_taplistio();
        send_to_google();
        send_to_mqtt();
    }
}

bool dataSendHandler::send_to_localTarget()
{
    bool result = true;

    if (data_sender.send_localTarget && !send_lock)
    {
        // Local Target
        send_localTarget = false;
        send_lock = true;
//        tilt_scanner.deinit();

        if (strlen(config.localTargetURL) >= LOCALTARGET_MIN_URL_LENGTH) {
            Log.verbose(F("Calling send to Local Target.\r\n"));
            DynamicJsonDocument doc(TILT_ALL_DATA_SIZE + 128);
            char tilt_data[TILT_ALL_DATA_SIZE + 128];

            // TODO - Refactor out tilt_to_json_string as this is the only place it is now used
            tilt_scanner.tilt_to_json_string(tilt_data, true);

            doc["mdns_id"] = config.mdnsID;
            doc["tilts"] = serialized(tilt_data);

            serializeJson(doc, tilt_data);

            if (send_to_url(config.localTargetURL, tilt_data, content_json))
            {
                Log.notice(F("Completed send to Local Target.\r\n"));
            }
            else
            {
                result = false; // There was an error with the previous send
                Log.verbose(F("Error sending to Local Target.\r\n"));
            }
        }
        localTargetTicker.once(config.localTargetPushEvery, [](){data_sender.send_localTarget = true;}); // Set up subsequent send to localTarget
//        tilt_scanner.init();
        send_lock = false;
    }
    return result;
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
        brewersFriendTicker.once(BREWERS_FRIEND_DELAY, [](){data_sender.send_brewersFriend = true;}); // Set up subsequent send to Brewer's Friend
        send_lock = false;
    }

    if (data_sender.send_brewfather && !send_lock)
    {
        send_lock = true;
        // Brewfather
        data_sender.send_brewfather = false;
        if (strlen(config.brewfatherKey) > BREWFATHER_MIN_KEY_LENGTH) {
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
        brewfatherTicker.once(BREWFATHER_DELAY, [](){data_sender.send_brewfather = true;}); // Set up subsequent send to Brewfather
        send_lock = false;
    }


    if (data_sender.send_userTarget && !send_lock)
    {
        send_lock = true;
        // User Target
        data_sender.send_userTarget = false;
        if (strlen(config.userTargetURL) > USER_TARGET_MIN_URL_LENGTH)
        {
            Log.verbose(F("Calling send to User Target.\r\n"));
            retval = data_sender.send_to_bf_and_bf(BF_MEANS_USER_TARGET);
            if (retval)
            {
                Log.notice(F("Completed send to User Target.\r\n"));
            }
            else
            {
                Log.verbose(F("Error sending to User Target.\r\n"));
            }
        }
        userTargetTicker.once(USER_TARGET_DELAY, [](){data_sender.send_userTarget = true;}); // Set up subsequent send to User Target
        send_lock = false;
    }
    return retval;
}

void dataSendHandler::send_to_cloud()
{
    if (data_sender.send_cloudTarget && !send_lock) {
        send_lock = true;
        send_cloudTarget = false;
        addTiltToParse();
        cloudTargetTicker.once(CLOUD_DELAY, [](){data_sender.send_cloudTarget = true;}); // Set up subsequent send to localTarget
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
    else if (which_bf == BF_MEANS_USER_TARGET)
    {
        if (strlen(config.userTargetURL) <= USER_TARGET_MIN_URL_LENGTH)
        {
            Log.verbose(F("User target URL not populated. Returning.\r\n"));
            return false;
        }
        strcpy(url, config.userTargetURL);
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
            char gravity[10];
            char temp[6];

            Log.verbose(F("Tilt loaded with color name: %s\r\n"), tilt_color_names[i]);
            j["name"] = tilt_color_names[i];
            tilt_scanner.tilt(i)->converted_temp(temp, sizeof(temp), true); // Always in Fahrenheit
            j["temp"] = temp;
            j["temp_unit"] = "F";
            tilt_scanner.tilt(i)->converted_gravity(gravity, sizeof(gravity), false);
            j["gravity"] = gravity;
            j["gravity_unit"] = "G";
            j["device_source"] = "TiltBridge";

            char payload_string[BF_SIZE];
            serializeJson(j, payload_string);

            if (!send_to_url(url, payload_string, content_json))
                result = false; // There was an error with the previous send
        }
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
                char gravity[10];
                char temp[6];
                StaticJsonDocument<GF_SIZE> j;
                Log.verbose(F("Tilt loaded with color name: %s\r\n"), tilt_color_names[i]);
                tilt_scanner.tilt(i)->converted_temp(temp, sizeof(temp), true); // Always in Fahrenheit
                j["Temp"] = temp;
                j["Unit"] = "F";
                tilt_scanner.tilt(i)->converted_gravity(gravity, sizeof(gravity), false);
                j["SG"] = gravity;

                char payload_string[GF_SIZE];
                serializeJson(j, payload_string);

                if (!send_to_url(config.grainfatherURL[i].link, payload_string, content_json))
                {
                    result = false; // There was an error with the previous send
                }
            }
        }
        grainfatherTicker.once(GRAINFATHER_DELAY, [](){data_sender.send_grainfather = true;}); // Set up subsequent send to Grainfather
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
        Log.verbose(F("taplist.io: send lock set.\r\n"));
        return false;
    }


    // Since we're only using .once timers, we can just detach/recreate every time and be fine
    taplistioTicker.detach();

    // Attempt to send.
    send_taplistio = false;
    send_lock = true;

    // This is now checked in the data sending loop
    // if (WiFiClass::status() != WL_CONNECTED) {
    //     Log.verbose(F("taplist.io: Wifi not connected, skipping send.\r\n"));
    //     taplistioTicker.once(config.taplistioPushEvery, [](){data_sender.send_taplistio = true;});
    //     send_lock = false;
    //     return false;
    // }

    for (uint8_t i = 0; i < TILT_COLORS; i++) {
        StaticJsonDocument<192> j;
        char payload_string[192];
        char gravity[10];
        char temp[6];


        if (!tilt_scanner.tilt(i)->is_loaded()) {
            continue;
        }

        j["Color"] = tilt_color_names[i];
        tilt_scanner.tilt(i)->converted_temp(temp, sizeof(temp), true);  // Always in Fahrenheit
        j["Temp"] = temp;
        tilt_scanner.tilt(i)->converted_gravity(gravity, sizeof(gravity), false);
        j["SG"] = gravity;
        j["temperature_unit"] = "F";
        j["gravity_unit"] = "G";
        
        serializeJson(j, payload_string);

        Log.verbose(F("taplist.io: Sending %s Tilt to %s\r\n"), tilt_color_names[i], config.taplistioURL);

        result = send_to_url(config.taplistioURL, payload_string, content_json);
    }

    taplistioTicker.once(config.taplistioPushEvery, [](){data_sender.send_taplistio = true;});
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
                    char gravity[10];
                    char temp[6];
                    tilt_scanner.tilt(i)->converted_gravity(gravity, sizeof(gravity), false);
                    tilt_scanner.tilt(i)->converted_temp(temp, sizeof(temp), true); // Always in Fahrenheit since we don't send units
                    snprintf(payload, payload_size, "SG=%s&Temp=%s&Color=%s&Timepoint=%.11f&Beer=Undefined&Comment=",
                            gravity, temp, tilt_color_names[i], ((double)std::time(0) + (config.TZoffset * 3600.0)) / 86400.0 + 25569.0);
                    
                    if (send_to_url(config.brewstatusURL, payload, content_x_www_form_urlencoded)) {
                        Log.notice(F("Completed send to Brew Status.\r\n"));
                    } else {
                        result = false;
                        Log.verbose(F("Error sending to Brew Status.\r\n"));
                    }
                }
            }
        }
        brewStatusTicker.once(config.brewstatusPushEvery, [](){data_sender.send_brewStatus = true;}); // Set up subsequent send to Brew Status
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
                        char gravity[10];
                        char temp[6];

                        // If there's a sheet name saved, then we should send the data
                        if (numSent == 0)
                            Log.notice(F("Beginning GSheets check-in.\r\n"));
                        payload["Beer"] = config.gsheets_config[i].name;
                        tilt_scanner.tilt(i)->converted_temp(temp, sizeof(temp), true); // Always in Fahrenheit
                        payload["Temp"] = temp;
                        tilt_scanner.tilt(i)->converted_gravity(gravity, sizeof(gravity), false);
                        payload["SG"] = gravity;
                        payload["Color"] = tilt_color_names[i];
                        payload["Comment"] = "";
                        payload["Email"] = config.scriptsEmail; // The gmail email address associated with the script on google
                        payload["tzOffset"] = config.TZoffset;

                        serializeJson(payload, payload_string);
                        payload.clear();

                        HTTPClient http;
                        WiFiClientSecure secureClient;

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

                            http.addHeader(F("Content-Type"), content_json);   // Specify content-type header
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

                                if(strcmp(config.gsheets_config[i].link, retval["doclongurl"].as<const char *>()) != 0) {
                                    Log.verbose(F("Storing new doclongurl: %s.\r\n"), retval["doclongurl"].as<const char *>());
                                    strlcpy(config.gsheets_config[i].link, retval["doclongurl"].as<const char *>(), 255);
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
        gSheetsTicker.once(GSCRIPTS_DELAY, [](){data_sender.send_gSheets = true;}); // Set up subsequent send to Google Sheets

        //tilt_scanner.init();
        send_lock = false;
    }
    return result;
}

void dataSendHandler::init_mqtt()
{
    LCBUrl url;

    // Checking for the WiFi Status is done in the data sending loop, but we also need to be sure we are connected to WiFi when we initialize the MQTT client
    if (WiFiClass::status() == WL_CONNECTED) {
        if(mqtt_alreadyinit) {
            Log.verbose(F("MQTT already initialized. Disconnecting.\r\n"));
            mqttClient.disconnect();
            delay(250);
        }

        if (strcmp(config.mqttBrokerHost, "") != 0 || strlen(config.mqttBrokerHost) != 0) {
            if (url.isMDNS(config.mqttBrokerHost)) {
                Log.verbose(F("Initializing connection to MQTTBroker: %s (%s) on port: %d\r\n"),
                    config.mqttBrokerHost, url.getIP(config.mqttBrokerHost).toString().c_str(), config.mqttBrokerPort);
            } else {
                Log.verbose(F("Initializing connection to MQTTBroker: %s on port: %d\r\n"),
                    config.mqttBrokerHost, config.mqttBrokerPort);
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
            mqttClient.setKeepAlive(config.mqttPushEvery);
        }
    }
}

void dataSendHandler::connect_mqtt()
{
    // Checking for the WiFi Status is done in the data sending loop, but we also need to be sure we are connected to WiFi when we connect to the MQTT broker
    if (WiFiClass::status() == WL_CONNECTED) {
        if(!mqtt_alreadyinit) {
            // Since init is not called synchronously with the settings update when the user sets the MQTT broker, we need to
            // wait until the MQTT client is initialized if it hasn't been done already.
            return;
        }
        if (strlen(config.mqttUsername) > 1) {
            mqttClient.connect(config.mdnsID, config.mqttUsername, config.mqttPassword);
        } else {
            mqttClient.connect(config.mdnsID);
        }
    }
}

String lcburl_getAfterPath(LCBUrl url) // Get anything after the path
{
    String afterpath = "";

    if (url.getQuery().length() > 0) {
        afterpath = "?" + url.getQuery();
    }

    if (url.getFragment().length() > 0) {
        afterpath = afterpath + "#" + url.getFragment();
    }

    return afterpath;
}

bool dataSendHandler::send_to_url(const char *url, const char *dataToSend, const char *contentType, bool checkBody, const char* bodyCheck)
{
    // This handles the generic act of sending data to an endpoint
    bool retVal = false;

    if (strlen(dataToSend) > 5 && strlen(url) > 8)
    {
        LCBUrl lcburl;
        lcburl.setUrl(url);

        bool validTarget = false;
        // There is an issue where the built-in HTTP client for some reason won't resolve mDNS addresses. Instead, we'll
        // resolve the address first, and then pass that to the client if needed. 
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
            // if (lcburl.getScheme() == "http")
                validTarget = true;
        }

        if (validTarget) {
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


            // This may crash if we're allocating too much memory on the heap, but if I can get away with this
            // it's the easiest solution.             
            HTTPClient http;
            WiFiClientSecure secureClient;
            secureClient.setInsecure(); // Don't perform certificate validation. This opens up MITM attacks, but I don't have memory otherwise.

            // Determine if the URL is HTTP or HTTPS and initialize HTTPClient
            if (lcburl.getScheme() == "https") {
                http.begin(secureClient, url); // HTTPS
            } else {
                http.begin(url); // HTTP
            }

            // Set headers
            http.addHeader(F("Content-Type"), contentType);
            http.addHeader(F("Accept"), content_json);

            char userAgent[128];
            snprintf(userAgent, sizeof(userAgent),
                "tiltbridge/%s (branch %s; build %s)",
                version(),
                branch(),
                build()
            );
            http.setUserAgent(userAgent);

            yield();  // Yield before we lock up the radio

            // Send the request
            int httpResponseCode;
            httpResponseCode = http.POST(dataToSend);

            // Optionally check the response
            bool result = false;
            if (httpResponseCode > 0) {
                // HTTP header has been sent and Server response header has been handled
                Log.verbose(F("HTTP Response code: %d\r\n"), httpResponseCode);

                if (checkBody) {
                    String response = http.getString();
                    if (response.indexOf(bodyCheck) >= 0) {
                        result = true;
                    } else {
                        Log.error(F("Body check failed. Body: %s\r\n"), response.c_str());
                    }
                } else {
                    result = (httpResponseCode == HTTP_CODE_OK);
                }
            } else {
                Log.error(F("Error on sending POST: %s\r\n"), http.errorToString(httpResponseCode).c_str());
                Log.error(F("Connection failed, Host: %s, Port: %l\r\n"), lcburl.getHost().c_str(), lcburl.getPort());
            }

            // Close connection
            http.end();
            delay(100);  // Give garbage collection time to run
            return result;

        } else {
            Log.error(F("Invalid target: %s.\r\n"), url);
        }
    } else {
        Log.notice(F("No URL provided, or no data to send.\r\n"));
    }
    // If we reached here, the send was unsuccessful
    return false;
}


bool dataSendHandler::send_to_mqtt() {
    bool result = false;

    if (strcmp(config.mqttBrokerHost, "") == 0 || strlen(config.mqttBrokerHost) == 0) {
        // No MQTT broker configured
        return false;
    }

    if (!mqttClient.connected()) {
        Log.warning(F("MQTT disconnected. Attempting to reconnect to MQTT Broker in loop\r\n"));
        connect_mqtt();
    } else {
        mqttClient.loop();
    }

    if (send_mqtt && !send_lock) {
        send_mqtt = false;
        send_lock = true;

        Log.verbose(F("Publishing available results to MQTT Broker.\r\n"));

        for (uint8_t i = 0; i < TILT_COLORS; i++) {
            if (tilt_scanner.tilt(i)->is_loaded()) {
                prepare_and_send_payloads(i);
            }
        }

        mqttTicker.once(config.mqttPushEvery, [](){ data_sender.send_mqtt = true; });
        send_lock = false;
    }

    return result;
}

void dataSendHandler::prepare_and_send_payloads(uint8_t tilt_index) {
    char tilt_topic[50] = {'\0'};
    snprintf(tilt_topic, 50, "%s/tilt_%s", config.mqttTopic, tilt_color_names[tilt_index]);

    // Prepare and send each of the four payloads
    prepare_temperature_payload(tilt_color_names[tilt_index], tilt_topic);
    prepare_gravity_payload(tilt_color_names[tilt_index], tilt_topic);
    prepare_battery_payload(tilt_color_names[tilt_index], tilt_topic);
    prepare_general_payload(tilt_index, tilt_topic);
}

void dataSendHandler::enrich_announcement(const char* topic, const char* tilt_color, StaticJsonDocument<512>& payload) {
    payload["stat_t"] = topic;
    char deviceName[20];
    snprintf(deviceName, sizeof(deviceName), "Tilt %s", tilt_color);
    payload["dev"]["name"] = deviceName;
    payload["dev"]["ids"] = tilt_color;
    payload["dev"]["mdl"] = "Tilt Hydrometer";
    payload["dev"]["mf"] = "Baron Brew Equipment LLC";
    payload["dev"]["sw"] = version();
    payload["dev"]["sa"] = "Brewery";  // Suggested Area

    char ip_address_url[25] = "http://";
    {
        char ip[16];
        sprintf(ip, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
        strncat(ip_address_url, ip, 16);
        strcat(ip_address_url, "/");
    }


    payload["dev"]["cu"] = ip_address_url;
    // model and hw_version could be added, but it would require the Tilt object to determine Tilt vs. Tilt Pro


    payload["json_attr_t"] = topic;
    payload["json_attr_tpl"] = "{ \"Uptime\": \"{{ value_json.timeStamp }}\" }\n";


}


void dataSendHandler::prepare_temperature_payload(const char* tilt_color, const char* tilt_topic) {
    //Home Assistant Config Topic for Temperature
    char m_topic[90];
    char tilt_sensor_name[35];
    char uniq_id[30];
    char unit[10] = "\u00b0"; // Unicode for degree symbol
    StaticJsonDocument<512> payload;

    // Construct the MQTT topic string for temperature
    sprintf(m_topic, "homeassistant/sensor/%s_tilt_%s/temperature/config", config.mqttTopic, tilt_color);

    // Set up payload fields
    strcat(unit, config.tempUnit); // Append temperature unit after degree symbol
    payload["dev_cla"] = "temperature";
    payload["unit_of_meas"] = unit;
    payload["ic"] = "mdi:thermometer-lines";
    
    // Construct sensor name
    snprintf(tilt_sensor_name, sizeof(tilt_sensor_name), "Tilt Temperature - %s", tilt_color);
    payload["name"] = tilt_sensor_name;

    // Value template
    payload["val_tpl"] = "{{value_json.Temp}}";

    // Unique ID
    snprintf(uniq_id, sizeof(uniq_id), "tiltbridge_tilt%sT", tilt_color);
    payload["uniq_id"] = uniq_id;

    enrich_announcement(tilt_topic, tilt_color, payload);
    // Serialize and publish
    publish_to_mqtt(m_topic, payload, true); // Retain flag set to true
}


void dataSendHandler::prepare_gravity_payload(const char* tilt_color, const char* tilt_topic) {
    //Home Assistant Config Topic for Sp Gravity
    char m_topic[90];
    char tilt_sensor_name[35];
    char uniq_id[30];
    StaticJsonDocument<512> payload;

    // Construct the MQTT topic string for specific gravity
    sprintf(m_topic, "homeassistant/sensor/%s_tilt_%sG/sp_gravity/config", config.mqttTopic, tilt_color);

    // Set up payload fields
    payload["unit_of_meas"] = "SG";
    payload["ic"] = "mdi:trending-down";
    
    // Construct sensor name
    snprintf(tilt_sensor_name, sizeof(tilt_sensor_name), "Tilt Specific Gravity - %s", tilt_color);
    payload["name"] = tilt_sensor_name;

    // Value template
    payload["val_tpl"] = "{{value_json.SG}}";

    // Unique ID
    snprintf(uniq_id, sizeof(uniq_id), "tiltbridge_tilt%sG", tilt_color);
    payload["uniq_id"] = uniq_id;

    enrich_announcement(tilt_topic, tilt_color, payload);
    // Serialize and publish
    publish_to_mqtt(m_topic, payload, true); // Retain flag set to true
}

void dataSendHandler::prepare_battery_payload(const char* tilt_color, const char* tilt_topic) {
    //Home Assistant Config Topic for Weeks On Battery
    char m_topic[90];
    char tilt_sensor_name[35];
    char uniq_id[30];
    StaticJsonDocument<512> payload;

    // Construct the MQTT topic string for weeks on battery
    sprintf(m_topic, "homeassistant/sensor/%s_tilt_%sWoB/weeks_on_battery/config", config.mqttTopic, tilt_color);

    // Set up payload fields
    payload["unit_of_meas"] = "weeks";
    payload["ic"] = "mdi:battery";
    
    // Construct sensor name
    snprintf(tilt_sensor_name, sizeof(tilt_sensor_name), "Tilt Weeks On Battery - %s", tilt_color);
    payload["name"] = tilt_sensor_name;

    // Value template
    payload["val_tpl"] = "{{value_json.WoB}}";

    // Unique ID
    snprintf(uniq_id, sizeof(uniq_id), "tiltbridge_tilt%sWoB", tilt_color);
    payload["uniq_id"] = uniq_id;

    enrich_announcement(tilt_topic, tilt_color, payload);
    // Serialize and publish
    publish_to_mqtt(m_topic, payload, true); // Retain flag set to true
}

void dataSendHandler::prepare_general_payload(uint8_t tilt_index, const char* tilt_topic) {
    //General payload with sensor data
    char m_topic[90];
    char gravity[10];
    char temp[6];
    char battery_str[4]; // large enough for 0-255 and the null terminator
    StaticJsonDocument<512> payload;
    tiltHydrometer* current_tilt = tilt_scanner.tilt(tilt_index);

    // Construct the MQTT topic string for general sensor data
    strcpy(m_topic, tilt_topic);

    // Populate payload with sensor data
    payload["Color"] = tilt_color_names[tilt_index];
    payload["timeStamp"] = (int)std::time(0);
    payload["fermunits"] = "SG";
    current_tilt->converted_gravity(gravity, 10, false);
    payload["SG"] = gravity;
    current_tilt->converted_temp(temp, 6, false);
    payload["Temp"] = temp;
    payload["tempunits"] = config.tempUnit;
    current_tilt->get_weeks_battery(battery_str, 4);
    payload["WoB"] = battery_str;

    // Serialize and publish
    publish_to_mqtt(m_topic, payload, false); // Retain flag set to false for general data
}


bool dataSendHandler::publish_to_mqtt(const char* topic, StaticJsonDocument<512>& payload, bool retain) {
    char payload_string[512];
    serializeJson(payload, payload_string);

    if (!mqttClient.connected()) {
        Log.warning(F("MQTT disconnected. Attempting to reconnect to MQTT Broker\r\n"));
        connect_mqtt();
    }

    bool result = mqttClient.publish(topic, payload_string, retain, 0);
    if(result) {
        Log.verbose(F("Published to MQTT\r\n"));
    } else {
        Log.error(F("Failed to publish to MQTT\r\n"));
    }
    delay(10);
    return result;
}

