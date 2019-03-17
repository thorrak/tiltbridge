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

#include <WiFiClientSecure.h>

#ifdef USE_SECURE_GSCRIPTS
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



dataSendHandler::dataSendHandler () {
    send_to_fermentrack_at =    45 * 1000; // Trigger the first send to Fermentrack 45 seconds out
    send_to_brewers_friend_at = 50 * 1000; // Trigger the first send to Fermentrack 50 seconds out
    send_to_google_at =         55 * 1000; // Trigger the first send to Google Sheets 55 seconds out
}


void setClock() {
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



void send_to_fermentrack() {
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
            Serial.print("Error on sending POST: ");
            Serial.println(httpResponseCode);
        }
        http.end();  //Free resources
    }
    j.clear();
}


#ifdef USE_SECURE_GSCRIPTS
WiFiClientSecure *secure_client;

void prep_send_secure() {
    secure_client = new WiFiClientSecure;
    secure_client -> setCACert(rootCACertificate);
}

void send_secure() {
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
#endif