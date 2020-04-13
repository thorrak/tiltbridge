//
// Created by John Beeler on 4/13/20.
//

#include "tiltBridge.h"
#include "SecureWithRedirects.h"

#include <Arduino.h>
#include <HTTPClient.h>

#include <WiFi.h>

#include <WiFiMulti.h>
#include <WiFiClientSecure.h>


SecureWithRedirects::SecureWithRedirects() {
    // Initialize secure_client
    secure_client = new WiFiClientSecure;
    https = new HTTPClient;
    redirects=0;
    use_get=false;
    const char* headerNames[] = { "Location", "Last-Modified" };
    https->collectHeaders(headerNames, sizeof(headerNames)/sizeof(headerNames[0]));
}



void SecureWithRedirects::end() {
    // Delete secure_client/https and free memory
    secure_client->stop();
    delete https;
    delete secure_client;
}


bool SecureWithRedirects::send_with_redirects(const char * original_url, const char *api_key, const char *data_to_send) {
    // TODO - Refactor this into the constructor or something
    url = original_url;
    apiKey=api_key;
    dataToSend=data_to_send;

#ifdef DEBUG_PRINTS
    Serial.print("[send_with_redirects] apiKey: ");
    Serial.println(apiKey);

    Serial.print("[send_with_redirects] dataToSend: ");
    Serial.println(dataToSend);
#endif

    return attempt_send();

}


bool SecureWithRedirects::attempt_send() {
    String response;
    String location_header;
    int httpResponseCode;

    if(redirects >= MAXIMUM_REDIRECTS) { // We've been redirected too many times
#ifdef DEBUG_PRINTS
        Serial.println("[SWR::attempt_send] Too many redirects - returning");
#endif
        return false;
    }


    Serial.print("[SWR::attempt_send] Current URL: `");
    Serial.print(url);
    Serial.println("`");

    https->begin(*secure_client, url);
    https->addHeader("Content-Type", "application/json");             //Specify content-type header
    if (apiKey) {
        https->addHeader("X-API-KEY", apiKey);  //Specify API key header
    }
    if(use_get)
        httpResponseCode = https->GET(); // The redirect type we got implies we shouldn't re-send the POST data
    else
        httpResponseCode = https->POST(dataToSend);   //Send the actual POST request

    switch(httpResponseCode) {
        case HTTP_CODE_OK:
        case HTTP_CODE_CREATED:
        case HTTP_CODE_ACCEPTED:
        case HTTP_CODE_NON_AUTHORITATIVE_INFORMATION:
        case HTTP_CODE_NO_CONTENT:
        case HTTP_CODE_PARTIAL_CONTENT:
            // These are success codes - Return true, as we were able to post
#ifdef DEBUG_PRINTS
            response = https->getString();      //Get the response to the request
            Serial.print("[SWR::attempt_send] Success - Code");
            Serial.println(httpResponseCode);   //Print return code
            Serial.println(response);           //Print request answer
#endif
            https->end();
            return true;

        case HTTP_CODE_FOUND:
        case HTTP_CODE_SEE_OTHER:
            // These are redirect codes (which is the whole reason that this function exists)
#ifdef DEBUG_PRINTS
            response = https->getString();                       //Get the response to the request
            Serial.print("[SWR::attempt_send] Redirected (use get next time) - Code ");
            Serial.println(httpResponseCode);   //Print return code
#endif

            if(https->hasHeader("Location")) {
#ifdef DEBUG_PRINTS
                location_header = https->header("Location");                       //Get the response to the request
                Serial.print("[SWR::attempt_send] Location Header: ");
                Serial.println(location_header);   //Print return code
#endif
                url = https->header("Location");
                https->end();
                redirects++;  // Increment redirects
                use_get = true;  // For these redirect codes, we have to switch to use GET (without post data)
                return attempt_send();  // Then call attempt_send again
            } else {
#ifdef DEBUG_PRINTS
                Serial.print("[SWR::attempt_send] No location header found! ");
#endif
                https->end();
                return false;
            }
        case HTTP_CODE_MOVED_PERMANENTLY:
        case HTTP_CODE_TEMPORARY_REDIRECT:
        case HTTP_CODE_PERMANENT_REDIRECT:
            // These are redirect codes (which is the whole reason that this function exists)
#ifdef DEBUG_PRINTS
            response = https->getString();                       //Get the response to the request
            Serial.print("[SWR::attempt_send] Redirected - Code ");
            Serial.println(httpResponseCode);   //Print return code
#endif

            if(https->hasHeader("Location")) {
#ifdef DEBUG_PRINTS
                location_header = https->header("Location");                       //Get the response to the request
                Serial.print("[SWR::attempt_send] Location Header: ");
                Serial.println(location_header);   //Print return code
#endif
                url = https->header("Location");
                https->end();
                redirects++;  // Increment redirects
                return attempt_send();  // Then call attempt_send again
            } else {
#ifdef DEBUG_PRINTS
                Serial.print("[SWR::attempt_send] No location header found! ");
#endif
                https->end();
                return false;
            }

        case HTTPC_ERROR_CONNECTION_LOST:
        case HTTPC_ERROR_CONNECTION_REFUSED:
        case HTTPC_ERROR_NOT_CONNECTED:
        case HTTPC_ERROR_READ_TIMEOUT:
            // We had some kind of connection issue. Treat it as a redirect, but keep the same URL/method
#ifdef DEBUG_PRINTS
            response = https->getString();                       //Get the response to the request
            Serial.print("[SWR::attempt_send] Connection error - Code ");
            Serial.println(httpResponseCode);   //Print return code
#endif
            https->end();
            redirects++;  // Increment redirects
            return attempt_send();  // Then call attempt_send again
        default:
#ifdef DEBUG_PRINTS
            response = https->getString();                       //Get the response to the request
            Serial.print("[SWR::attempt_send] Failed - Code ");
            Serial.println(httpResponseCode);   //Print return code
            Serial.println(response);           //Print request answer
#endif
            https->end();
            return false;
    }

}