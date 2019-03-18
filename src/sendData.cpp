//
// Created by John Beeler on 2/18/19.
//

#include <nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;


#include "tiltBridge.h"
#include "wifi_setup.h"
#include "sendData.h"
#include <Arduino.h>
#include <HTTPClient.h>

#include <WiFi.h>
#include <WiFiMulti.h>


#ifdef USE_SECURE_GSCRIPTS
#include <WiFiClientSecure.h>
// This is the GlobalSign 2021 root cert (the one used by script.google.com)
// An appropriate root cert can be discovered for any site by running:
//   openssl s_client -showcerts -connect script.google.com:443 </dev/null'
// The CA root cert is the last cert given in the chain of certs
const char* rootCACertificate = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDujCCAqKgAwIBAgILBAAAAAABD4Ym5g0wDQYJKoZIhvcNAQEFBQAwTDEgMB4G\n" \
"A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjIxEzARBgNVBAoTCkdsb2JhbFNp\n" \
"Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDYxMjE1MDgwMDAwWhcNMjExMjE1\n" \
"MDgwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMjETMBEG\n" \
"A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI\n" \
"hvcNAQEBBQADggEPADCCAQoCggEBAKbPJA6+Lm8omUVCxKs+IVSbC9N/hHD6ErPL\n" \
"v4dfxn+G07IwXNb9rfF73OX4YJYJkhD10FPe+3t+c4isUoh7SqbKSaZeqKeMWhG8\n" \
"eoLrvozps6yWJQeXSpkqBy+0Hne/ig+1AnwblrjFuTosvNYSuetZfeLQBoZfXklq\n" \
"tTleiDTsvHgMCJiEbKjNS7SgfQx5TfC4LcshytVsW33hoCmEofnTlEnLJGKRILzd\n" \
"C9XZzPnqJworc5HGnRusyMvo4KD0L5CLTfuwNhv2GXqF4G3yYROIXJ/gkwpRl4pa\n" \
"zq+r1feqCapgvdzZX99yqWATXgAByUr6P6TqBwMhAo6CygPCm48CAwEAAaOBnDCB\n" \
"mTAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUm+IH\n" \
"V2ccHsBqBt5ZtJot39wZhi4wNgYDVR0fBC8wLTAroCmgJ4YlaHR0cDovL2NybC5n\n" \
"bG9iYWxzaWduLm5ldC9yb290LXIyLmNybDAfBgNVHSMEGDAWgBSb4gdXZxwewGoG\n" \
"3lm0mi3f3BmGLjANBgkqhkiG9w0BAQUFAAOCAQEAmYFThxxol4aR7OBKuEQLq4Gs\n" \
"J0/WwbgcQ3izDJr86iw8bmEbTUsp9Z8FHSbBuOmDAGJFtqkIk7mpM0sYmsL4h4hO\n" \
"291xNBrBVNpGP+DTKqttVCL1OmLNIG+6KYnX3ZHu01yiPqFbQfXf5WRDLenVOavS\n" \
"ot+3i9DAgBkcRcAtjOj4LaR0VknFBbVPFd5uRHg5h6h+u/N5GJG79G+dwfCMNYxd\n" \
"AfvDbbnvRG15RjF+Cv6pgsH/76tuIMRQyV+dTZsXjAzlAcmgQWpzU/qlULRuJQ/7\n" \
"TBj0/VLZjmmx6BEP3ojY+x1J96relc8geMJgEtslQIxq/H5COEBkEveegeGTLg==\n" \
"-----END CERTIFICATE-----\n";
#endif

dataSendHandler data_sender;  // Global data sender

dataSendHandler::dataSendHandler() {
    send_to_fermentrack_at =    45 * 1000; // Trigger the first send to Fermentrack 45 seconds out
    send_to_brewers_friend_at = 50 * 1000; // Trigger the first send to Brewer's Friend 50 seconds out
    send_to_google_at =         55 * 1000; // Trigger the first send to Google Sheets 55 seconds out
}


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


void dataSendHandler::init() {
    setClock();
#ifdef USE_SECURE_GSCRIPTS
    prep_send_secure();
#endif
}


void dataSendHandler::send_to_fermentrack() {
    HTTPClient http;
    nlohmann::json j;

    // This should look like this when sent to Fermentrack:
    // {
    //   'api_key': 'Key Goes Here',
    //   'tilts': {'color': 'Purple', 'temp': 74, 'gravity': 1.043},
    //            {'color': 'Orange', 'temp': 66, 'gravity': 1.001}
    // }

    j["tilts"] = tilt_scanner.tilt_to_json();
    j["api_key"] = app_config.config["fermentrackToken"].get<std::string>();


    if(strlen(j.dump().c_str()) > 5) {
#ifdef DEBUG_PRINTS
        Serial.print("Data to send: ");
        Serial.println(j.dump().c_str());
#endif

        http.begin(app_config.config["fermentrackURL"].get<std::string>().c_str());  //Specify destination for HTTP request
        http.addHeader("Content-Type", "application/json");             //Specify content-type header
        int httpResponseCode = http.POST(j.dump().c_str());   //Send the actual POST request

        if (httpResponseCode > 0) {
//            String response = http.getString();                       //Get the response to the request
//            Serial.println(httpResponseCode);   //Print return code
//            Serial.println(response);           //Print request answer
        } else {
#ifdef DEBUG_PRINTS
            Serial.print("Error on sending POST: ");
            Serial.println(httpResponseCode);
#endif
        }
        http.end();  //Free resources
    }
    j.clear();
}


#ifdef USE_SECURE_GSCRIPTS
WiFiClientSecure *secure_client;

void dataSendHandler::prep_send_secure() {
    secure_client = new WiFiClientSecure;
    secure_client -> setCACert(rootCACertificate);
}

void dataSendHandler::send_to_google() {
    nlohmann::json j;

    j["Beer"] = "some beer,2";
    j["Temp"] = 75;
    j["SG"] = (float) 1.050;
    j["Color"] = "Blue";
    j["Comment"] = "xxx@yyy.com";  // The gmail email address associated with the script on google
    j["Timepoint"] = (float) 42728.4267217361;

    if(secure_client) {

        { // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *secure_client is
            HTTPClient https;

            Serial.print("[HTTPS] begin...\r\n");
            if (https.begin(*secure_client, "https://script.google.com/a/optictheory.com/macros/s/AKfycbyqEM6l1c3Jh-1r5HPJ7ANWCKQcoUR2GDJMOcbX-mYFlyK49-EZ/exec")) {  // HTTPS
                Serial.print("[HTTPS] POST...\r\n");
                https.addHeader("Content-Type", "application/json");             //Specify content-type header
                // start connection and send HTTP header
                int httpCode = https.POST(j.dump().c_str());   //Send the actual POST request

                // httpCode will be negative on error
                if (httpCode > 0) {
                    // HTTP header has been send and Server response header has been handled
                    Serial.printf("[HTTPS] GET... code: %d\r\n", httpCode);

                    // file found at server
                    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                        Serial.println("OK");
//                        String payload = https.getString();
//                        Serial.println(payload);
                    }
                } else {
                    Serial.printf("[HTTPS] GET... failed, error: %s\r\n", https.errorToString(httpCode).c_str());
                }

                https.end();
            } else {
                Serial.println("[HTTPS] Unable to connect");
            }

            // End extra scoping block
        }

        // There are memory leaks when we do this, disabling creation/deletion of the secure_client for every round
//        delete secure_client;
    } else {
        Serial.println("Unable to create secure_client");
    }

    Serial.println();
    j.clear();
}
#else

void dataSendHandler::send_to_google() {
    HTTPClient http;
    nlohmann::json j;
    nlohmann::json payload;

    // There are two configuration options which are mandatory when using the Google Sheets integration
    if(app_config.config["scriptsURL"].get<std::string>().length() <= 12 || app_config.config["scriptsEmail"].get<std::string>().length() < 7) {
//#ifdef DEBUG_PRINTS
//        Serial.println("Either scriptsURL or scriptsEmail not populated. Returning.");
//#endif
        return;
    }
    

    // This should look like this when sent to the proxy that sends to Google (once per Tilt):
    // {
    //   'payload': {
    //        'Beer':     'Key Goes Here',
    //        'Temp':     65,
    //        'SG':       1.050,  // This is sent as a float
    //        'Color':    'Blue',
    //        'Comment':  '',
    //        'Email':    'xxx@gmail.com',
    //    },
    //   'gscripts_url': 'https://script.google.com/.../',  // This is specific to the proxy
    // }


    // Loop through each of the tilt colors cached by tilt_scanner, sending data for each of the active tilts
    for(uint8_t i = 0;i<TILT_COLORS;i++) {
        if(tilt_scanner.tilt(i)->is_loaded()) {
            if(tilt_scanner.tilt(i)->gsheets_beer_name().length() <= 0) {
//#ifdef DEBUG_PRINTS
//                Serial.print("Tilt has no beer name: ");
//                Serial.println(tilt_scanner.tilt(i)->color_name().c_str());
//#endif
                continue; // If there is no gsheets beer name, we don't know where to log to. Skip this tilt.
            }

//#ifdef DEBUG_PRINTS
//            Serial.print("Tilt loaded with beer name: ");
//            Serial.println(tilt_scanner.tilt(i)->color_name().c_str());
//#endif


            payload["Beer"] = tilt_scanner.tilt(i)->gsheets_beer_name();
            payload["Temp"] = tilt_scanner.tilt(i)->temp;  // Always in Fahrenheit
            payload["SG"] = (float) tilt_scanner.tilt(i)->gravity / 1000;
            payload["Color"] = tilt_scanner.tilt(i)->color_name();
            payload["Comment"] = "";
            payload["Email"] = app_config.config["scriptsEmail"].get<std::string>(); // The gmail email address associated with the script on google

            j["gscripts_url"] = app_config.config["scriptsURL"].get<std::string>();
            j["payload"] = payload;

            if(strlen(j.dump().c_str()) > 5) {
//#ifdef DEBUG_PRINTS
//                Serial.print("Data to send: ");
//                Serial.println(j.dump().c_str());
//#endif

                http.begin("http://www.tiltbridge.com/tiltbridge_google_proxy/");  //Specify destination for HTTP request
                http.addHeader("Content-Type", "application/json");             //Specify content-type header
                int httpResponseCode = http.POST(j.dump().c_str());   //Send the actual POST request

                if (httpResponseCode > 0) {
//#ifdef DEBUG_PRINTS
//                    String response = http.getString();                       //Get the response to the request
//                    Serial.println(httpResponseCode);   //Print return code
//                    Serial.println(response);           //Print request answer
//                } else {
//                    Serial.print("Error on sending POST: ");
//                    Serial.println(httpResponseCode);
//#endif
                }
                http.end();  //Free resources
            }
            payload.clear();
            j.clear();
        } else {
//#ifdef DEBUG_PRINTS
//        Serial.print("Tilt not loaded: ");
//        Serial.println(tilt_scanner.tilt(i)->color_name().c_str());
//#endif
        }
    }

}

#endif


void dataSendHandler::send_to_brewers_friend() {
    HTTPClient http;
    nlohmann::json j;
    std::string url;

    // Brewers Friend only requires a single option (API key)
    if(app_config.config["brewersFriendKey"].get<std::string>().length() <= 12) {
#ifdef DEBUG_PRINTS
        Serial.println("brewersFriendKey not populated. Returning.");
#endif
        return;
    }


    // This should look like this when sent to the proxy that sends to Brewers Friend (once per Tilt):
    // {
    //   'name':     'Key Goes Here',
    //        'Temp':     65,
    //        'SG':       1.050,  // This is sent as a float
    //        'Color':    'Blue',
    //        'Comment':  '',
    //        'Email':    'xxx@gmail.com',
    //   'gscripts_url': 'https://script.google.com/.../',  // This is specific to the proxy
    // }


    // Loop through each of the tilt colors cached by tilt_scanner, sending data for each of the active tilts
    for(uint8_t i = 0;i<TILT_COLORS;i++) {
        if(tilt_scanner.tilt(i)->is_loaded()) {
#ifdef DEBUG_PRINTS
            Serial.print("Tilt loaded with beer name: ");
            Serial.println(tilt_scanner.tilt(i)->color_name().c_str());
#endif
            j["name"] = tilt_scanner.tilt(i)->color_name();
            j["temp"] = tilt_scanner.tilt(i)->temp;  // Always in Fahrenheit
            j["temp_unit"] = "F";
            j["gravity"] = tilt_scanner.tilt(i)->converted_gravity();
            j["gravity_unit"] = "G";


            if(strlen(j.dump().c_str()) > 5) {
#ifdef DEBUG_PRINTS
                Serial.print("Data to send: ");
                Serial.println(j.dump().c_str());
#endif

                url = "http://log.brewersfriend.com/stream/" + app_config.config["brewersFriendKey"].get<std::string>();

                http.begin(url.c_str());  //Specify destination for HTTP request
                http.addHeader("Content-Type", "application/json");             //Specify content-type header
                http.addHeader("X-API-KEY", app_config.config["brewersFriendKey"].get<std::string>().c_str());  //Specify API key header
                int httpResponseCode = http.POST(j.dump().c_str());   //Send the actual POST request

                if (httpResponseCode > 0) {
#ifdef DEBUG_PRINTS
                    String response = http.getString();                       //Get the response to the request
                    Serial.println(httpResponseCode);   //Print return code
                    Serial.println(response);           //Print request answer
                } else {
                    Serial.print("Error on sending POST: ");
                    Serial.println(httpResponseCode);
#endif
                }
                http.end();  //Free resources
            }
            j.clear();
        } else {
#ifdef DEBUG_PRINTS
            Serial.print("Tilt not loaded: ");
            Serial.println(tilt_scanner.tilt(i)->color_name().c_str());
#endif
        }
    }

}




void dataSendHandler::process() {
    // dataSendHandler::process() processes each tick & dispatches HTTP clients to push data out as necessary

    // Check & send to Fermentrack if necessary
    if(send_to_fermentrack_at <= xTaskGetTickCount()) {
        if(WiFi.status()== WL_CONNECTED && app_config.config["fermentrackURL"].get<std::string>().length() > 12) {   //Check WiFi connection status
#ifdef DEBUG_PRINTS
            Serial.printf("Calling send to Fermentrack\r\n");
#endif
            // tilt_scanner.wait_until_scan_complete();
            send_to_fermentrack();
        }
        send_to_fermentrack_at = xTaskGetTickCount() + (app_config.config["fermentrackPushEvery"].get<int>() * 1000);
        yield();
    }

    // Check & send to Google Scripts if necessary
    if(send_to_google_at <= xTaskGetTickCount()) {
        if(WiFi.status()== WL_CONNECTED && app_config.config["scriptsURL"].get<std::string>().length() > 12) {
#ifdef DEBUG_PRINTS
            Serial.printf("Calling send to Google\r\n");
#endif
            // tilt_scanner.wait_until_scan_complete();
            send_to_google();
        }
        send_to_google_at = xTaskGetTickCount() + GSCRIPTS_DELAY;
        yield();
    }

    // Check & send to Brewers Friend if necessary
    if(send_to_brewers_friend_at <= xTaskGetTickCount()) {
        if(WiFi.status()== WL_CONNECTED && app_config.config["brewersFriendKey"].get<std::string>().length() > 12) {
#ifdef DEBUG_PRINTS
            Serial.printf("Calling send to Brewers Friend\r\n");
#endif
            // tilt_scanner.wait_until_scan_complete();
            send_to_brewers_friend();
        }
        send_to_brewers_friend_at = xTaskGetTickCount() + BREWERS_FRIEND_DELAY;
        yield();
    }


}
