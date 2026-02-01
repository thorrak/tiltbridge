#include <ctime>
#include <ArduinoJson.h>
#include <Ticker.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include "mqtt_client.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include <thorlog.h>
#include "url_utils.h"

#include "tilt/tiltScanner.h"
#include "jsonconfig.h"
#include "version.h"
#include "http_server.h"
#include "main.h"  // for printMem()

#include "sendData.h"


dataSendHandler data_sender; // Global data sender

dataSendHandler::dataSendHandler() {}

void dataSendHandler::init()
{
    init_mqtt();

    // Set up timers
    legacyFermentrackTicker.once(12, [](){data_sender.send_legacy_fermentrack = true;});      // Schedule first send to Legacy Fermentrack
    fermentrackTicker.once(10, [](){data_sender.send_fermentrack = true;});      // Schedule first send to Fermentrack
    mqttTicker.once(20, [](){data_sender.send_mqtt = true;});                    // Schedule first send to MQTT
    brewStatusTicker.once(30, [](){data_sender.send_brewStatus = true;});        // Schedule first send to Brew Status
    brewfatherTicker.once(40, [](){data_sender.send_brewfather = true;});        // Schedule first send to Brewfather
    brewersFriendTicker.once(50, [](){data_sender.send_brewersFriend = true;});  // Schedule first send to Brewer's Friend
    userTargetTicker.once(60, [](){data_sender.send_userTarget = true;});        // Schedule first send to User-defined JSON target
    gSheetsTicker.once(70, [](){data_sender.send_gSheets = true;});              // Schedule first send to Google Sheets
    grainfatherTicker.once(80, [](){data_sender.send_grainfather = true;});      // Schedule first send to Grainfather
    taplistioTicker.once(90, [](){data_sender.send_taplistio = true;});          // Schedule first send to Taplist.io
    influxdbTicker.once(100, [](){data_sender.send_influxdb = true;});           // Schedule first send to InfluxDB
}

void dataSendHandler::process()
{
    if (WiFi.status() == WL_CONNECTED) {
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

bool dataSendHandler::send_to_legacy_fermentrack()
{
    bool result = true;

    if (send_legacy_fermentrack && !send_lock)
    {
        // Fermentrack
        send_legacy_fermentrack = false;
        send_lock = true;
//        tilt_scanner.deinit();

        if (strlen(config.legacyFermentrackURL) >= FERMENTRACK_MIN_URL_LENGTH) {
            Log.verbose("Calling send to Legacy Fermentrack.\r\n");
            JsonDocument doc;
            char tilt_data[TILT_ALL_DATA_SIZE + 128];

            // Load the Tilt data from the scanner
            JsonDocument tilt_doc;
            // This is the only call to tilt_to_json_legacy
            // The main difference vs tilt_to_json is that it sends a dict with the color as the key rather than an array.
            // When we discontinue Legacy Fermentrack support this can also be discontinued
            // This also only ever sends raw gravity
            tilt_scanner.tilt_to_json_legacy(tilt_doc);

            doc["mdns_id"] = config.mdnsID;
            doc["tilts"] = tilt_doc;

            serializeJson(doc, tilt_data);

            if (send_to_url(config.legacyFermentrackURL, tilt_data, content_json))
            {
                Log.notice("Completed send to Legacy Fermentrack.\r\n");
            }
            else
            {
                result = false; // There was an error with the previous send
                Log.verbose("Error sending to Legacy Fermentrack.\r\n");
            }
        }
        legacyFermentrackTicker.once(config.legacyFermentrackPushEvery, [](){data_sender.send_legacy_fermentrack = true;}); // Set up subsequent send to Fermentrack
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
        brewersFriendTicker.once(BREWERS_FRIEND_DELAY, [](){data_sender.send_brewersFriend = true;}); // Set up subsequent send to Brewer's Friend
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
        userTargetTicker.once(USER_TARGET_DELAY, [](){data_sender.send_userTarget = true;}); // Set up subsequent send to User Target
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

        if (!send_to_url(url, payload_string, content_json))
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

            if (!send_to_url(config.grainfatherURL[th.m_color].link, payload_string, content_json))
                result = false; // There was an error with the previous send
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
        Log.verbose("taplist.io: send lock set.\r\n");
        return false;
    }


    // Since we're only using .once timers, we can just detach/recreate every time and be fine
    taplistioTicker.detach();

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
                
                if (send_to_url(config.brewstatusURL, payload, content_x_www_form_urlencoded)) {
                    Log.notice("Completed send to Brew Status.\r\n");
                } else {
                    result = false;
                    Log.verbose("Error sending to Brew Status.\r\n");
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
        JsonDocument payload;
        char payload_string[GSHEETS_JSON];
        JsonDocument retval;
        int httpResponseCode;
        int numSent = 0;
#if (ARDUINO_LOG_LEVEL == 6)
        char buff[1024] = "";
#endif

        // The google sheets handler only fires if we have both a Google Scripts URL to post to, and an email address.
        if (strlen(config.scriptsURL) >= GSCRIPTS_MIN_URL_LENGTH && strlen(config.scriptsEmail) >= GSCRIPTS_MIN_EMAIL_LENGTH) {
            Log.verbose("Checking for any pending Google Sheets pushes.\r\n");
//            Log.verbose("Executing on core %i.\r\n", xPortGetCoreID());
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

                    HTTPClient http;
                    WiFiClientSecure secureClient;

                    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);  // Follow the 301
                    http.setConnectTimeout(6000);                           // Set 6 second timeout
                    http.setTimeout(10000);                                 // Set 10 second timeout
                    http.setReuse(false);
                    secureClient.setInsecure();                             // Ignore SHA fingerprint

                    if (!http.begin(secureClient, config.scriptsURL)) {      // Connect secure
                        Log.error("Unable to create secure connection to %s.\r\n", config.scriptsURL);
                        result = false;
                    } else {
                        // Failed to open a connection
                        Log.verbose("Created secure connection to %s.\r\n", config.scriptsURL);
                        Log.verbose("Sending the following payload to Google Sheets (%s):\r\n\t\t%s\r\n", tilt_color_names[th.m_color], payload_string);

                        http.addHeader("Content-Type", content_json);   // Specify content-type header
                        httpResponseCode = http.POST(payload_string);               // Send the payload

                        if (httpResponseCode == HTTP_CODE_OK) {  // HTTP_CODE_OK = 200
                            // POST success
#if (ARDUINO_LOG_LEVEL == 6)
                            // We need to use a buffer in order to be able to use the response twice
                            strlcpy(buff, http.getString().c_str(), 1024);
                            Log.verbose("HTTP Response: 200\r\nFull Response:\r\n\t%s\r\n", buff);
                            deserializeJson(retval, buff);
//                                deserializeJson(retval, http.getString().c_str());
#else
                            deserializeJson(retval, http.getString().c_str());
#endif

                            if(strcmp(config.gsheets_config[th.m_color].link, retval["doclongurl"].as<const char *>()) != 0) {
                                Log.verbose("Storing new doclongurl: %s.\r\n", retval["doclongurl"].as<const char *>());
                                strlcpy(config.gsheets_config[th.m_color].link, retval["doclongurl"].as<const char *>(), 255);
                                config.save();
                            }
                            retval.clear();
                            numSent++;
                        } else {
                            // Post generated an error (response code != 200)
                            Log.error("Google send to %s Tilt failed (%d): %s. Response:\r\n%s\r\n",
                                tilt_color_names[th.m_color],
                                httpResponseCode,
                                http.errorToString(httpResponseCode).c_str(),
                                http.getString().c_str());
                            result = false;
                        } // Response code != 200
                    } // Good connection
                    http.end();
                    delay(100);  // Give garbage collection time to run
                } // Check we have a sheet name for the color
            }

            Log.notice("Submitted %l sheet%s to Google.\r\n", numSent, (numSent== 1) ? "" : "s");

        }
        gSheetsTicker.once(GSCRIPTS_DELAY, [](){data_sender.send_gSheets = true;}); // Set up subsequent send to Google Sheets

        //tilt_scanner.init();
        send_lock = false;
    }
    return result;
}

void dataSendHandler::mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    dataSendHandler *self = static_cast<dataSendHandler*>(handler_args);
    esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);

    switch (static_cast<esp_mqtt_event_id_t>(event_id)) {
        case MQTT_EVENT_CONNECTED:
            Log.notice("MQTT connected to broker.\r\n");
            self->mqtt_connected = true;
            break;
        case MQTT_EVENT_DISCONNECTED:
            Log.warning("MQTT disconnected from broker.\r\n");
            self->mqtt_connected = false;
            break;
        case MQTT_EVENT_ERROR:
            Log.error("MQTT error occurred.\r\n");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                Log.error("MQTT transport error: %d\r\n", event->error_handle->esp_transport_sock_errno);
            }
            break;
        default:
            break;
    }
}

void dataSendHandler::init_mqtt()
{
    // Checking for the WiFi Status is done in the data sending loop, but we also need to be sure we are connected to WiFi when we initialize the MQTT client
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    // If already initialized, stop and destroy the existing client
    if (mqtt_alreadyinit && mqtt_client != nullptr) {
        Log.verbose("MQTT already initialized. Stopping client.\r\n");
        esp_mqtt_client_stop(mqtt_client);
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = nullptr;
        mqtt_connected = false;
        delay(250);
    }

    // Check if broker is configured
    if (strcmp(config.mqttBrokerHost, "") == 0 && strlen(config.mqttBrokerHost) == 0) {
        return;
    }

    // Resolve mDNS hostname if needed
    char broker_host[256];
    IPAddress resolvedIP;
    bool mdnsHost = isMDNS(config.mqttBrokerHost);

    if (mdnsHost) {
        resolveHost(config.mqttBrokerHost, resolvedIP);
        // Convert IPAddress to string for esp-mqtt
        snprintf(broker_host, sizeof(broker_host), "%d.%d.%d.%d",
                 resolvedIP[0], resolvedIP[1], resolvedIP[2], resolvedIP[3]);
        Log.verbose("Initializing connection to MQTTBroker: %s (%s) on port: %d\r\n",
            config.mqttBrokerHost, broker_host, config.mqttBrokerPort);
    } else {
        strncpy(broker_host, config.mqttBrokerHost, sizeof(broker_host) - 1);
        broker_host[sizeof(broker_host) - 1] = '\0';
        Log.verbose("Initializing connection to MQTTBroker: %s on port: %d\r\n",
            config.mqttBrokerHost, config.mqttBrokerPort);
    }

    // Build the esp-mqtt configuration
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.hostname = broker_host;
    mqtt_cfg.broker.address.port = config.mqttBrokerPort;
    mqtt_cfg.broker.address.transport = MQTT_TRANSPORT_OVER_TCP;
    mqtt_cfg.credentials.client_id = config.mdnsID;
    mqtt_cfg.session.keepalive = config.mqttPushEvery;
    mqtt_cfg.buffer.size = 512;
    mqtt_cfg.buffer.out_size = 512;

    // Set username/password if configured
    if (strlen(config.mqttUsername) > 1) {
        mqtt_cfg.credentials.username = config.mqttUsername;
        mqtt_cfg.credentials.authentication.password = config.mqttPassword;
    }

    // Create the MQTT client
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == nullptr) {
        Log.error("Failed to initialize MQTT client.\r\n");
        return;
    }

    // Register event handler
    esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, this);

    // Start the client
    esp_err_t err = esp_mqtt_client_start(mqtt_client);
    if (err != ESP_OK) {
        Log.error("Failed to start MQTT client: %d\r\n", err);
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = nullptr;
        return;
    }

    mqtt_alreadyinit = true;
    Log.verbose("MQTT client started.\r\n");
}

void dataSendHandler::connect_mqtt()
{
    // esp-mqtt handles auto-reconnection internally, so this function primarily
    // ensures the client is initialized and can force a reconnection if needed
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    if (!mqtt_alreadyinit || mqtt_client == nullptr) {
        // Client not initialized yet, init_mqtt will handle connection
        return;
    }

    // Force a reconnection attempt if disconnected
    if (!mqtt_connected) {
        esp_mqtt_client_reconnect(mqtt_client);
    }
}

bool dataSendHandler::send_to_url(const char *url, const char *dataToSend, const char *contentType, bool checkBody, const char* bodyCheck)
{
    // This handles the generic act of sending data to an endpoint
    bool retVal = false;
    bool result = false;
    bool https = false;

    if (strlen(dataToSend) > 5 && strlen(url) > 8)
    {
        bool validTarget = false;
        ParsedUrl parsedUrl;
        parseUrl(url, &parsedUrl);

        // There is an issue where the built-in HTTP client for some reason won't resolve mDNS addresses. Instead, we'll
        // resolve the address first, and then pass that to the client if needed.
        IPAddress resolvedIP;
        if (isMDNS(parsedUrl.host))
        {
            // Make sure we can resolve the address
            if (resolveHost(parsedUrl.host, resolvedIP) && resolvedIP != INADDR_NONE)
                validTarget = true;
        }
        else if (isValidIP(parsedUrl.host))
            // We were passed an IP Address
            validTarget = true;
        else
        {
            // If it's not mDNS all we care about is that it's http
            // if (strcmp(parsedUrl.scheme, "http") == 0)
                validTarget = true;
        }
        if (validTarget) {
            if (isMDNS(parsedUrl.host))
                // Use the IP address we resolved (necessary for mDNS)
                Log.verbose("Connecting to: %s at %s on port %l\r\n",
                            parsedUrl.host,
                            resolvedIP.toString().c_str(),
                            parsedUrl.port);
            else
                Log.verbose("Connecting to: %s on port %l\r\n",
                            parsedUrl.host,
                            parsedUrl.port);
        }
        if (strcmp(parsedUrl.scheme, "https") == 0)
            https=true;

        if (validTarget) {
            WiFiClientSecure *secureClient;
            secureClient = new WiFiClientSecure();
            {
                // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
                HTTPClient *http;
                http = new HTTPClient();
                secureClient->setInsecure(); // Don't perform certificate validation. This opens up MITM attacks, but I don't have memory otherwise.

                // Determine if the URL is HTTP or HTTPS and initialize HTTPClient
                if (https) {
                    http->begin(*secureClient, url); // HTTPS
                } else {
                    http->begin(url); // HTTP
                }

                // Set headers
                http->addHeader("Content-Type", contentType);
                http->addHeader("Accept", content_json);

                char userAgent[128];
                snprintf(userAgent, sizeof(userAgent), "tiltbridge/%s (branch %s; build %s)", version(), branch(), build());
                http->setUserAgent(userAgent);

                yield();  // Yield before we lock up the radio

                // Send the request
                Log.verbose("Sent data: %s\r\n", dataToSend);
                int httpResponseCode;
                // httpResponseCode = http->sendRequest("POST", dataToSend);
                httpResponseCode = http->POST(dataToSend);

                // Optionally check the response
                if (httpResponseCode > 0) {
                    // HTTP header has been sent and Server response header has been handled
                    Log.verbose("HTTP Response code: %d\r\n", httpResponseCode);

                    if (checkBody) {
                        String response = http->getString();
                        if (response.indexOf(bodyCheck) >= 0) {
                            result = true;
                        } else {
                            Log.error("Body check failed. Body: %s\r\n", response.c_str());
                        }
                    } else {
                        result = (httpResponseCode == HTTP_CODE_OK);
                    }
                } else {
                    Log.error("Error on sending POST: %s\r\n", http->errorToString(httpResponseCode).c_str());
                    Log.error("Connection failed\r\n");
                }

                // Close connection
                http->end();
                delay(100);  // Give garbage collection time to run
                delete http;
            }
            secureClient->stop();
            delete secureClient;

            return result;

        } else {
            Log.error("Invalid target: %s.\r\n", url);
        }
    } else {
        Log.notice("No URL provided, or no data to send.\r\n");
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

    // esp-mqtt handles connection and reconnection internally via events
    // We just check the connection status and optionally trigger a reconnect
    if (!mqtt_connected && mqtt_client != nullptr) {
        Log.warning("MQTT disconnected. Triggering reconnect attempt.\r\n");
        connect_mqtt();
    }

    if (send_mqtt && !send_lock) {
        send_mqtt = false;
        send_lock = true;

        Log.verbose("Publishing available results to MQTT Broker.\r\n");

        tilt_scanner.drop_expired_tilts();

        for(tiltHydrometer & th : tilt_scanner.m_tilt_devices) {
            char tilt_topic[50] = {'\0'};
            snprintf(tilt_topic, 50, "%s/tilt_%s", config.mqttTopic, tilt_color_names[th.m_color]);

            // Prepare and send each of the four payloads
            prepare_temperature_payload(&th, tilt_topic);
            prepare_gravity_payload(&th, tilt_topic);
            prepare_battery_payload(&th, tilt_topic);
            prepare_general_payload(&th, tilt_topic);
        }

        mqttTicker.once(config.mqttPushEvery, [](){ data_sender.send_mqtt = true; });
        send_lock = false;
    }

    return result;
}


void dataSendHandler::enrich_announcement(const char* topic, const char* tilt_color, JsonDocument& payload) {
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


void dataSendHandler::prepare_temperature_payload(tiltHydrometer *th, const char* tilt_topic) {
    //Home Assistant Config Topic for Temperature
    char m_topic[90];
    char tilt_sensor_name[35];
    char uniq_id[30];
    char unit[10] = "\u00b0"; // Unicode for degree symbol
    JsonDocument payload;

    // Construct the MQTT topic string for temperature
    sprintf(m_topic, "homeassistant/sensor/%s_tilt_%s/temperature/config", config.mqttTopic, tilt_color_names[th->m_color]);

    // Set up payload fields
    strcat(unit, config.tempUnit); // Append temperature unit after degree symbol
    payload["dev_cla"] = "temperature";
    payload["unit_of_meas"] = unit;
    payload["ic"] = "mdi:thermometer-lines";
    
    // Construct sensor name
    snprintf(tilt_sensor_name, sizeof(tilt_sensor_name), "Tilt Temperature - %s", tilt_color_names[th->m_color]);
    payload["name"] = tilt_sensor_name;

    // Value template
    payload["val_tpl"] = "{{value_json.Temp}}";

    // Unique ID
    snprintf(uniq_id, sizeof(uniq_id), "tiltbridge_tilt%sT", tilt_color_names[th->m_color]);
    payload["uniq_id"] = uniq_id;

    enrich_announcement(tilt_topic, tilt_color_names[th->m_color], payload);
    // Serialize and publish
    publish_to_mqtt(m_topic, payload, true); // Retain flag set to true
}


void dataSendHandler::prepare_gravity_payload(tiltHydrometer *th, const char* tilt_topic) {
    //Home Assistant Config Topic for Sp Gravity
    char m_topic[90];
    char tilt_sensor_name[35];
    char uniq_id[30];
    JsonDocument payload;

    // Construct the MQTT topic string for specific gravity
    sprintf(m_topic, "homeassistant/sensor/%s_tilt_%sG/sp_gravity/config", config.mqttTopic, tilt_color_names[th->m_color]);

    // Set up payload fields
    payload["unit_of_meas"] = "SG";
    payload["ic"] = "mdi:trending-down";
    
    // Construct sensor name
    snprintf(tilt_sensor_name, sizeof(tilt_sensor_name), "Tilt Specific Gravity - %s", tilt_color_names[th->m_color]);
    payload["name"] = tilt_sensor_name;

    // Value template
    payload["val_tpl"] = "{{value_json.SG}}";

    // Unique ID
    snprintf(uniq_id, sizeof(uniq_id), "tiltbridge_tilt%sG", tilt_color_names[th->m_color]);
    payload["uniq_id"] = uniq_id;

    enrich_announcement(tilt_topic, tilt_color_names[th->m_color], payload);
    // Serialize and publish
    publish_to_mqtt(m_topic, payload, true); // Retain flag set to true
}

void dataSendHandler::prepare_battery_payload(tiltHydrometer *th, const char* tilt_topic) {
    //Home Assistant Config Topic for Weeks On Battery
    char m_topic[90];
    char tilt_sensor_name[35];
    char uniq_id[30];
    JsonDocument payload;

    // Construct the MQTT topic string for weeks on battery
    sprintf(m_topic, "homeassistant/sensor/%s_tilt_%sWoB/weeks_on_battery/config", config.mqttTopic, tilt_color_names[th->m_color]);

    // Set up payload fields
    payload["unit_of_meas"] = "weeks";
    payload["ic"] = "mdi:battery";
    
    // Construct sensor name
    snprintf(tilt_sensor_name, sizeof(tilt_sensor_name), "Tilt Weeks On Battery - %s", tilt_color_names[th->m_color]);
    payload["name"] = tilt_sensor_name;

    // Value template
    payload["val_tpl"] = "{{value_json.WoB}}";

    // Unique ID
    snprintf(uniq_id, sizeof(uniq_id), "tiltbridge_tilt%sWoB", tilt_color_names[th->m_color]);
    payload["uniq_id"] = uniq_id;

    enrich_announcement(tilt_topic, tilt_color_names[th->m_color], payload);
    // Serialize and publish
    publish_to_mqtt(m_topic, payload, true); // Retain flag set to true
}

void dataSendHandler::prepare_general_payload(tiltHydrometer *th, const char* tilt_topic) {
    //General payload with sensor data
    char m_topic[90];
    char gravity[10];
    char temp[6];
    char battery_str[4]; // large enough for 0-255 and the null terminator
    JsonDocument payload;

    // Construct the MQTT topic string for general sensor data
    strcpy(m_topic, tilt_topic);

    // Populate payload with sensor data
    payload["Color"] = tilt_color_names[th->m_color];
    payload["timeStamp"] = (int)std::time(0);
    payload["fermunits"] = "SG";
    th->cal_smooth_gravity_str(gravity, sizeof(gravity));
    payload["SG"] = gravity;
    th->converted_temp(temp, 6, false);
    payload["Temp"] = temp;
    payload["tempunits"] = config.tempUnit;
    th->get_weeks_battery(battery_str, 4);
    payload["WoB"] = battery_str;

    // Serialize and publish
    publish_to_mqtt(m_topic, payload, false); // Retain flag set to false for general data
}


bool dataSendHandler::publish_to_mqtt(const char* topic, JsonDocument& payload, bool retain) {
    char payload_string[512];
    serializeJson(payload, payload_string);

    if (!mqtt_connected || mqtt_client == nullptr) {
        Log.warning("MQTT disconnected. Attempting to reconnect to MQTT Broker\r\n");
        connect_mqtt();
        // If still not connected, we can't publish
        if (!mqtt_connected) {
            Log.error("Failed to publish to MQTT - not connected\r\n");
            return false;
        }
    }

    // esp_mqtt_client_publish returns message_id on success (>=0), -1 on failure
    // Parameters: client, topic, data, len (0 = use strlen), qos (0), retain (0 or 1)
    int msg_id = esp_mqtt_client_publish(mqtt_client, topic, payload_string, 0, 0, retain ? 1 : 0);
    bool result = (msg_id >= 0);
    if (result) {
        Log.verbose("Published to MQTT (msg_id=%d)\r\n", msg_id);
    } else {
        Log.error("Failed to publish to MQTT\r\n");
    }
    delay(10);
    return result;
}

bool dataSendHandler::send_to_influxdb()
{
    bool result = true;

    if (send_influxdb && !send_lock)
    {
        send_influxdb = false;
        send_lock = true;

        if (strlen(config.influxdbURL) > INFLUXDB_MIN_URL_LENGTH && 
            strlen(config.influxdbToken) > 0 && 
            strlen(config.influxdbOrg) > 0 && 
            strlen(config.influxdbBucket) > 0) {
            
            Log.verbose("Calling send to InfluxDB.\r\n");
            
            // Build the write API URL
            char writeURL[512];
            snprintf(writeURL, sizeof(writeURL), "%s/api/v2/write?org=%s&bucket=%s&precision=s", 
                     config.influxdbURL, config.influxdbOrg, config.influxdbBucket);

            // Build line protocol data
            String lineData = "";
            uint64_t timestamp = std::time(nullptr);
            
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
                snprintf(line, sizeof(line), 
                        "tilt,color=%s,device_source=TiltBridge "
                        "gravity=%s,temperature=%s,temp_units=\"%s\",weeks_on_battery=%s\n",
                        tilt_color_names[th.m_color],
                        gravity, temp, config.tempUnit, battery_str);
                
                lineData += line;
            }

            if (lineData.length() > 0) {
                // Send data using HTTP POST with line protocol
                HTTPClient http;
                WiFiClientSecure secureClient;

                // Determine if URL is HTTPS
                bool useHTTPS = strncmp(config.influxdbURL, "https://", 8) == 0;
                
                if (useHTTPS) {
                    secureClient.setInsecure(); // Don't verify certificates
                    http.begin(secureClient, writeURL);
                } else {
                    http.begin(writeURL);
                }

                // Set headers for InfluxDB v2 API
                http.addHeader("Authorization", String("Token ") + config.influxdbToken);
                http.addHeader("Content-Type", "text/plain; charset=utf-8");
                http.addHeader("Accept", "application/json");

                char userAgent[128];
                snprintf(userAgent, sizeof(userAgent), "tiltbridge/%s (branch %s; build %s)", 
                         version(), branch(), build());
                http.setUserAgent(userAgent);

                // Send the data
                int httpResponseCode = http.POST(lineData);

                if (httpResponseCode > 0) {
                    Log.verbose("InfluxDB HTTP Response code: %d\r\n", httpResponseCode);
                    
                    if (httpResponseCode >= 200 && httpResponseCode < 300) {
                        Log.notice("Completed send to InfluxDB.\r\n");
                    } else {
                        Log.error("InfluxDB returned error code %d: %s\r\n", 
                                 httpResponseCode, http.getString().c_str());
                        result = false;
                    }
                } else {
                    Log.error("Error sending to InfluxDB: %s\r\n", 
                             http.errorToString(httpResponseCode).c_str());
                    result = false;
                }

                http.end();
                if (useHTTPS) {
                    secureClient.stop();
                }
            } else {
                Log.verbose("No Tilt data to send to InfluxDB.\r\n");
            }
        }
        
        influxdbTicker.once(config.influxdbPushEvery, [](){data_sender.send_influxdb = true;}); // Set up subsequent send to InfluxDB
        send_lock = false;
    }
    return result;
}

