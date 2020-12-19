//
// Created by John Beeler on 2/18/19.
// Modified by Tim Pletcher on 31-Oct-2020.
//

#include<ctime>

#include <nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;

#include "tiltBridge.h"
#include "wifi_setup.h"
#include "sendData.h"
#include <Arduino.h>
#include <HTTPClient.h>

#include <WiFi.h>
#include <MQTT.h>

#ifdef USE_SECURE_GSCRIPTS
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include "SecureWithRedirects.h"
#endif

dataSendHandler data_sender;  // Global data sender

WiFiClient wClient;
MQTTClient mqttClient;

dataSendHandler::dataSendHandler() {
    send_to_brewstatus_at =     40 * 1000; // Trigger first send to BrewStatus 40 seconds out
    send_to_fermentrack_at =    45 * 1000; // Trigger first send to Fermentrack 45 seconds out
    send_to_brewfather_at =     50 * 1000; // Trigger first send to Fermentrack 50 seconds out
    send_to_brewers_friend_at = 55 * 1000; // Trigger first send to Brewer's Friend 55 seconds out
    send_to_mqtt_at =           60 * 1000; // Trigger first send to MQTT 60 seconds out
    send_to_google_at =         70 * 1000; // Trigger first send to Google Sheets 70 seconds out
#ifdef ENABLE_TEST_CHECKINS
    send_checkin_at =           35 * 1000; // If we have send_checkins enabled (this is a testing thing!) send at 35 seconds
#endif

    mqtt_alreadyinit = false;
}


#ifdef USE_SECURE_GSCRIPTS

void dataSendHandler::setClock() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    time_t nowSecs = time(nullptr);
    while (nowSecs < 8 * 3600 * 2) {
        delay(500);
        yield();
        nowSecs = time(nullptr);
    }

    struct tm timeinfo;
    gmtime_r(&nowSecs, &timeinfo);
}

#endif


void dataSendHandler::init() {
#ifdef USE_SECURE_GSCRIPTS
    setClock();
#endif
}

void dataSendHandler::init_mqtt() {
    if(app_config.config["mqttBrokerIP"].get<std::string>().length() > IP_MIN_STRING_LENGTH){
#ifdef DEBUG_PRINTS
        Serial.print(F("Initializing Connection to MQTTBroker at IP: "));
        Serial.print(app_config.config["mqttBrokerIP"].get<std::string>().c_str());
        Serial.print(F(" on port: "));
        Serial.println(app_config.config["mqttBrokerPort"].get<int>());
#endif
        const char * mqttserver = app_config.config["mqttBrokerIP"].get<std::string>().c_str();
        mqttClient.setKeepAlive(app_config.config["mqttPushEvery"].get<int>() * 1000);

        if (mqtt_alreadyinit) {
            mqttClient.disconnect();
            delay(250);  
            mqttClient.setHost(mqttserver,app_config.config["mqttBrokerPort"].get<int>());
        } else {
            mqttClient.begin(mqttserver,app_config.config["mqttBrokerPort"].get<int>(),wClient);
        }
        mqtt_alreadyinit=true;   
    }
}

void dataSendHandler::connect_mqtt() {
    if (app_config.config["mqttUsername"].get<std::string>().length()>1){
        mqttClient.connect(app_config.config["mdnsID"].get<std::string>().c_str(),app_config.config["mqttUsername"].get<std::string>().c_str(),app_config.config["mqttPassword"].get<std::string>().c_str());
    } else {
        mqttClient.connect(app_config.config["mdnsID"].get<std::string>().c_str());      
    }   
}

bool dataSendHandler::send_to_fermentrack() {
    nlohmann::json j;
    bool result = true;

    // This should look like this when sent to Fermentrack:
    // {
    //   'mdns_id': 'mDNS ID Goes Here',
    //   'tilts': {'color': 'Purple', 'temp': 74, 'gravity': 1.043},
    //            {'color': 'Orange', 'temp': 66, 'gravity': 1.001}
    // }

    j["mdns_id"] = app_config.config["mdnsID"].get<std::string>();
    j["tilts"] = tilt_scanner.tilt_to_json();


    if(!send_to_url(app_config.config["fermentrackURL"].get<std::string>().c_str(), "", j.dump().c_str(), "application/json"))
        result = false;  // There was an error with the previous send

    j.clear();
    return result;
}


bool dataSendHandler::send_to_brewstatus() {
    bool result = true;
    const int payload_size = 512;
    char payload[payload_size];

    // The payload should look like this when sent to Brewstatus:
    // ('Request payload:', 'SG=1.019&Temp=71.0&Color=ORANGE&Timepoint=43984.33630927084&Beer=Beer&Comment=Comment')
    // BrewStatus ignors Beer, so we just set this to Undefined.
    // BrewStatus will record Comment if it set, but just leave it blank.
    // The Timepoint is Google Sheets time, which is fractional days since 12/30/1899
    // Using https://www.timeanddate.com/date/durationresult.html?m1=12&d1=30&y1=1899&m2=1&d2=1&y2=1970 gives
    // us 25,569 days from the start of Google Sheets time to the start of the Unix epoch.
    // BrewStatus wants local time, so we allow the user to specify a time offset.

    // Loop through each of the tilt colors cached by tilt_scanner, sending data for each of the active tilts
    for(uint8_t i = 0;i<TILT_COLORS;i++) {
        if(tilt_scanner.tilt(i)->is_loaded()) {
            snprintf(payload, payload_size, "SG=%f&Temp=%f&Color=%s&Timepoint=%.11f&Beer=Undefined&Comment=", 
                     (float) tilt_scanner.tilt(i)->gravity / 1000,
                     (float) tilt_scanner.tilt(i)->temp,
                     tilt_scanner.tilt(i)->color_name().c_str(),
                     ((double) std::time(0) + (app_config.config["brewstatusTZoffset"].get<double>() * 3600.0))
                     / 86400.0 + 25569.0);
            if(!send_to_url(app_config.config["brewstatusURL"].get<std::string>().c_str(), "", payload, "application/x-www-form-urlencoded"))
                result = false;  // There was an error with the previous send
        }
    }

    return result;
}


#ifdef USE_SECURE_GSCRIPTS
// For sending data to Google Scripts, we have to use secure_client but otherwise we're doing the same thing as before.
bool dataSendHandler::send_to_url_https(const char *url, const char *apiKey, const char *dataToSend, const char *contentType) {
    // This handles the generic act of sending data to an endpoint
    bool result = false;

    if (strlen(dataToSend) > 5) {
#ifdef DEBUG_PRINTS
        Serial.print("[HTTPS] Sending data to: ");
        Serial.println(url);
        Serial.print("Data to send: ");
        Serial.println(dataToSend);
        Serial.printf("[HTTPS] Pre-deinit RAM left %d\r\n", esp_get_free_heap_size());
#endif
        // We're severely memory starved. Deinitialize bluetooth and free the related memory
        // NOTE - This is not strictly true under NimBLE. Deinit now only waits for a scan to complete before
        tilt_scanner.deinit();
        yield();

#ifdef DEBUG_PRINTS
        Serial.printf("[HTTPS] Post-deinit RAM left %d\r\n", esp_get_free_heap_size());
        Serial.println("[HTTPS] Calling SWR::send_with_redirects");
#endif

        SecureWithRedirects SWR(url, apiKey, dataToSend, contentType);
        result = SWR.send_with_redirects();
        SWR.end();

#ifdef DEBUG_PRINTS
        Serial.printf("[HTTPS] Post-SWR RAM left %d\r\n", esp_get_free_heap_size());
#endif
        yield();
        tilt_scanner.init();
        yield();
#ifdef DEBUG_PRINTS
        Serial.printf("[HTTPS] Post-reinit RAM left %d\r\n", esp_get_free_heap_size());
#endif
    }
    return result;
}

#endif

bool dataSendHandler::send_to_google() {
    HTTPClient http;
    nlohmann::json j;
    nlohmann::json payload;
    bool result = true;

    // There are two configuration options which are mandatory when using the Google Sheets integration
    if(app_config.config["scriptsURL"].get<std::string>().length() <= GSCRIPTS_MIN_URL_LENGTH ||
       app_config.config["scriptsEmail"].get<std::string>().length() < GSCRIPTS_MIN_EMAIL_LENGTH) {
//#ifdef DEBUG_PRINTS
//        Serial.println("Either scriptsURL or scriptsEmail not populated. Returning.");
//#endif
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
    for(uint8_t i = 0;i<TILT_COLORS;i++) {
        if(tilt_scanner.tilt(i)->is_loaded()) {
            if(tilt_scanner.tilt(i)->gsheets_beer_name().length() <= 0) {
                continue; // If there is no gsheets beer name, we don't know where to log to. Skip this tilt.
            }

            payload["Beer"] = tilt_scanner.tilt(i)->gsheets_beer_name();
            payload["Temp"] = tilt_scanner.tilt(i)->temp;  // Always in Fahrenheit
            payload["SG"] = tilt_scanner.tilt(i)->converted_gravity();
            payload["Color"] = tilt_scanner.tilt(i)->color_name();
            payload["Comment"] = "";
            payload["Email"] = app_config.config["scriptsEmail"].get<std::string>(); // The gmail email address associated with the script on google

#ifdef USE_SECURE_GSCRIPTS
            // When sending the data to GScripts directly, we're sending the payload - not the wrapped payload
            if(!send_to_url_https(app_config.config["scriptsURL"].get<std::string>().c_str(), "", payload.dump().c_str(), "application/json"))
                result = false;  // There was an error with the previous send
            payload.clear();
#else
            j["gscripts_url"] = app_config.config["scriptsURL"].get<std::string>();
            j["payload"] = payload;

            // All data for non-secure gscripts goes through the TiltBridge google proxy script. I'm not happy with this
            // but it's the best I've got until HTTPS can be readded
            if(!send_to_url("http://www.tiltbridge.com/tiltbridge_google_proxy/", "", j.dump().c_str(), "application/json"))
                result = false;  // There was an error with the previous send
            payload.clear();
            j.clear();
#endif
        }
    }
    return result;
}




bool dataSendHandler::send_to_bf_and_bf(const uint8_t which_bf) {
    // This function combines the data formatting for both "BF"s - Brewers Friend & Brewfather
    // Once the data is formatted, it is dispatched to send_to_url to be sent out.

    bool result = true;
    nlohmann::json j;
    std::string apiKeyStr;
    std::string url;

    // As this function is being used for both Brewer's Friend and Brewfather, let's determine which we want and set up
    // the URL/API key accordingly.
    if(which_bf == BF_MEANS_BREWFATHER) {
        apiKeyStr = app_config.config["brewfatherKey"].get<std::string>();
        if (apiKeyStr.length() <= BREWFATHER_MIN_KEY_LENGTH) {
#ifdef DEBUG_PRINTS
            Serial.println("brewfatherKey not populated. Returning.\r\n");
#endif
            return false;
        }
        url = "http://log.brewfather.net/stream?id=" + apiKeyStr;
    } else if(which_bf == BF_MEANS_BREWERS_FRIEND) {
        apiKeyStr = app_config.config["brewersFriendKey"].get<std::string>();
        if(apiKeyStr.length() <= BREWERS_FRIEND_MIN_KEY_LENGTH) {
#ifdef DEBUG_PRINTS
            Serial.println("brewersFriendKey not populated. Returning.");
#endif
            return false;
        }
        url = "http://log.brewersfriend.com/stream/" + apiKeyStr;
    } else {
#ifdef DEBUG_PRINTS
        Serial.println("Invalid value of which_bf passed to send_to_bf_and_bf!");
#endif
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
    for(uint8_t i = 0;i<TILT_COLORS;i++) {
        if(tilt_scanner.tilt(i)->is_loaded()) {
#ifdef DEBUG_PRINTS
            Serial.print("Tilt loaded with color name: ");
            Serial.println(tilt_scanner.tilt(i)->color_name().c_str());
#endif
            j["name"] = tilt_scanner.tilt(i)->color_name();
            j["temp"] = tilt_scanner.tilt(i)->temp;  // Always in Fahrenheit
            j["temp_unit"] = "F";
            j["gravity"] = tilt_scanner.tilt(i)->converted_gravity();
            j["gravity_unit"] = "G";
            j["device_source"] = "TiltBridge";

            if(!send_to_url(url.c_str(), apiKeyStr.c_str(), j.dump().c_str(), "application/json"))
                result = false;  // There was an error with the previous send

            j.clear();
        }
    }
    return result;
}



bool dataSendHandler::send_to_url(const char *url, const char *apiKey, const char *dataToSend, const char *contentType) {
    // This handles the generic act of sending data to an endpoint
    HTTPClient http;
    bool result = false;

    if(strlen(dataToSend) > 5) {
        #ifdef DEBUG_PRINTS
            Serial.print("Sending data to: ");
            Serial.println(url);
            Serial.print("Data to send: ");
            Serial.println(dataToSend);
        #endif

        // If we're really memory starved, we can wait to send any data until current scans complete
        // tilt_scanner.wait_until_scan_complete();

        http.begin(url);
        http.addHeader("Content-Type", contentType);             //Specify content-type header
        if (apiKey) {
            http.addHeader("X-API-KEY", apiKey);  //Specify API key header
        }
        int httpResponseCode = http.POST(dataToSend);   //Send the actual POST request

        if (httpResponseCode > 0) {
            result = true;
#ifdef DEBUG_PRINTS
            String response = http.getString();                       //Get the response to the request
            Serial.println(httpResponseCode);   //Print return code
            Serial.println(response);           //Print request answer
        } else {
                Serial.print("Error on sending POST: ");
                Serial.println(httpResponseCode);   //Print return code
#endif
        }
        http.end();  //Free resources
    }

    return result;
}

bool dataSendHandler::send_to_mqtt() {
    bool result = false;
    const int payload_size = 70;
    char payload[payload_size];
    mqttClient.loop();
    delay(10);

    // The payload formatted to look like this when sent to mqTT:
    // ('topic,tilt_color=Black specific_gravity=1.001,temperature=78.00')
    // This is compatible with influxdb format when used with influxdb/telegraf.
    //
    // Loop through each of the tilt colors cached by tilt_scanner, sending data for each of the active tilts
    for(uint8_t i = 0;i<TILT_COLORS;i++) {
        if(tilt_scanner.tilt(i)->is_loaded()) {
            snprintf(payload, payload_size,"%s,tilt_color=%s specific_gravity=%.3f,temperature=%.0f",
                    app_config.config["mqttTopic"].get<std::string>().c_str(), 
                    tilt_scanner.tilt(i)->color_name().c_str(),
                    (float) tilt_scanner.tilt(i)->gravity / 1000,
                    (float) tilt_scanner.tilt(i)->temp);
            //std::string pang_payload = payload;
#ifdef DEBUG_PRINTS                    
            Serial.print(F("Topic: "));
            Serial.println(app_config.config["mqttTopic"].get<std::string>().c_str());
            Serial.print(F("Message: "));
            Serial.println(payload);
#endif
            if (!mqttClient.connected()) {
#ifdef DEBUG_PRINTS
                Serial.println(F("MQTT disconnected. Attempting to reconnect to MQTT Broker"));
#endif                
                connect_mqtt();
                delay(500);               
            }
            result = mqttClient.publish(app_config.config["mqttTopic"].get<std::string>().c_str(),payload);
#ifdef DEBUG_PRINTS
                Serial.print(F("Publish Successful: "));
                Serial.println(result);
#endif 
        }

    }
    return result;
}

#ifdef ENABLE_TEST_CHECKINS
u_long checkin_no = 0;

void send_checkin_stat() {
    HTTPClient http;


    String Data = "checkin_no=";
    Data += String(checkin_no);
//    Data += "\r\n\r\n";


//    Serial.print("Data to send: ");
//    Serial.println(Data);

    http.begin("http://www.fermentrack.com/checkin/");  //Specify destination for HTTP request
    http.addHeader("Content-Type", "application/json");             //Specify content-type header
    int httpResponseCode = http.POST(Data);   //Send the actual POST request

    if (httpResponseCode > 0) {
//        String response = http.getString();                       //Get the response to the request
//        Serial.println(httpResponseCode);   //Print return code
//        Serial.println(response);           //Print request answer
    } else {
//        Serial.print("Error on sending POST: ");
//        Serial.println(httpResponseCode);
    }
    http.end();  //Free resources

    checkin_no = checkin_no + 1;
}
#endif

void dataSendHandler::process() {
    // dataSendHandler::process() processes each tick & dispatches HTTP clients to push data out as necessary

    // Check & send to Fermentrack if necessary
    if(send_to_fermentrack_at <= xTaskGetTickCount()) {
        if(WiFiClass::status()== WL_CONNECTED && app_config.config["fermentrackURL"].get<std::string>().length() > FERMENTRACK_MIN_URL_LENGTH) {   //Check WiFi connection status
            #ifdef DEBUG_PRINTS
                Serial.printf("Calling send to Fermentrack\r\n");
            #endif

            send_to_fermentrack();
            send_to_fermentrack_at = xTaskGetTickCount() + (app_config.config["fermentrackPushEvery"].get<int>() * 1000);
        } else {
            // If the user adds the setting, we want this to kick in within 10 seconds
            send_to_fermentrack_at = xTaskGetTickCount() + 10000;
        }
        yield();
    }

    // Check & send to Brewstatus if necessary
    if(send_to_brewstatus_at <= xTaskGetTickCount()) {
        if(WiFiClass::status()== WL_CONNECTED && app_config.config["brewstatusURL"].get<std::string>().length() > BREWSTATUS_MIN_URL_LENGTH) {   //Check WiFi connection status
            #ifdef DEBUG_PRINTS
                Serial.printf("Calling send to Brewstatus\r\n");
            #endif

            send_to_brewstatus();
            send_to_brewstatus_at = xTaskGetTickCount() + (app_config.config["brewstatusPushEvery"].get<int>() * 1000);
        } else {
            // If the user adds the setting, we want this to kick in within 10 seconds
            send_to_brewstatus_at = xTaskGetTickCount() + 10000;
        }
        yield();
    }

    // Check & send to Google Scripts if necessary
    if(send_to_google_at <= xTaskGetTickCount()) {
        if(WiFiClass::status()== WL_CONNECTED && app_config.config["scriptsURL"].get<std::string>().length() > GSCRIPTS_MIN_URL_LENGTH) {
#ifdef DEBUG_PRINTS
            Serial.printf("Calling send to Google\r\n");
#endif
            // tilt_scanner.wait_until_scan_complete();
            send_to_google();
            send_to_google_at = xTaskGetTickCount() + GSCRIPTS_DELAY;
        } else {
            // If the user adds the setting, we want this to kick in within 10 seconds
            send_to_google_at = xTaskGetTickCount() + 10000;
        }
        yield();
    }

    // Check & send to Brewers Friend if necessary
    if(send_to_brewers_friend_at <= xTaskGetTickCount()) {
        if(WiFiClass::status()== WL_CONNECTED && app_config.config["brewersFriendKey"].get<std::string>().length() > BREWERS_FRIEND_MIN_KEY_LENGTH) {
            #ifdef DEBUG_PRINTS
                Serial.printf("Calling send to Brewers Friend\r\n");
            #endif

            send_to_bf_and_bf(BF_MEANS_BREWERS_FRIEND);
            send_to_brewers_friend_at = xTaskGetTickCount() + BREWERS_FRIEND_DELAY;
        } else {
            // If the user adds the setting, we want this to kick in within 10 seconds
            send_to_brewers_friend_at = xTaskGetTickCount() + 10000;
        }
        yield();
    }

#ifdef ENABLE_TEST_CHECKINS
    if(send_checkin_at <= xTaskGetTickCount()) {
        if(WiFiClass::status()== WL_CONNECTED) {   //Check WiFi connection status
#ifdef DEBUG_PRINTS
            Serial.printf("Calling check-in to fermentrack.com\r\n");
#endif
            // tilt_scanner.wait_until_scan_complete();
            send_checkin_stat();
        }
        send_checkin_at = xTaskGetTickCount() + (60*5 * 1000);
        yield();
    }
#endif

    if (send_to_brewfather_at <= xTaskGetTickCount()) {
        if(WiFiClass::status() == WL_CONNECTED && app_config.config["brewfatherKey"].get<std::string>().length() > BREWFATHER_MIN_KEY_LENGTH) {
            #ifdef DEBUG_PRINTS
                Serial.printf("Calling send to Brewfather\r\n");
            #endif

            send_to_bf_and_bf(BF_MEANS_BREWFATHER);
            send_to_brewfather_at = xTaskGetTickCount() + BREWFATHER_DELAY;
        } else {
            // If the user adds the setting, we want this to kick in within 10 seconds
            send_to_brewfather_at = xTaskGetTickCount() + 10000;
        }
        yield();
    }

        // Check & send to mqtt broker if necessary
    if (send_to_mqtt_at <= xTaskGetTickCount()) {
        if(WiFiClass::status()== WL_CONNECTED && app_config.config["mqttBrokerIP"].get<std::string>().length() > IP_MIN_STRING_LENGTH) {   //Check WiFi connection status
            #ifdef DEBUG_PRINTS
                Serial.println(F("Publishing available results to MQTT Broker"));
            #endif

            send_to_mqtt();
            send_to_mqtt_at = xTaskGetTickCount() + (app_config.config["mqttPushEvery"].get<int>() * 1000);
        } else {
            // If the user adds the setting, we want this to kick in within 10 seconds
            send_to_mqtt_at = xTaskGetTickCount() + 10000;
        }
        yield();
    }
}
