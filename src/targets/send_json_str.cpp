
#include <ArduinoLog.h>

#include "send_json_str.h"
#include "version.h"


constexpr const char* httpMethodToString(httpMethod method) {
    switch (method) {
        case httpMethod::HTTP_PUT:
            return "PUT";
        case httpMethod::HTTP_POST:
            return "POST";
        case httpMethod::HTTP_PATCH:
            return "PATCH";
        case httpMethod::HTTP_DELETE:
            return "DELETE";
        case httpMethod::HTTP_GET:
        default:
            return "GET";
    }
}

void get_useragent(char *ua, size_t size) {
    snprintf(ua, size,
        "tiltbridge/%s (commit %s)",
        version(),
        build()
    );
}

sendResult send_json_str(String &payload, const char *url, httpMethod method) {
    String response;
    return send_json_str(payload, url, response, method);
}

sendResult send_json_str(String &payload, const char *url, String &response, httpMethod method) {
    char auth_header[64];
    char userAgent[128];
    int httpResponseCode;
    sendResult result;

    // send_lock = true;


    if (WiFi.status() != WL_CONNECTED) {
        Log.warning(F("send_json_str: Wifi not connected, skipping send.\r\n"));
        // send_lock = false;
        return sendResult::retry;
    }

    get_useragent(userAgent, sizeof(userAgent));

    // snprintf(auth_header, sizeof(auth_header), "token %s", config.secret);
   
    Log.info(F("send_json_str: Sending %s to %s\r\n"), payload.c_str(), url);

    yield();  // Yield before we lock up the radio

    // TODO - Determine if we can get rid of the call to new
    // WiFiClientSecure *client = new WiFiClientSecure;
    WiFiClient client;
    if(true) {
        // client.setInsecure();
        {
            // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
            HTTPClient http;

            http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
#ifndef ESP8266
            http.setConnectTimeout(6000);
#endif
            http.setReuse(false);

            if (http.begin(client, url)) {
                http.addHeader(F("Content-Type"), F("application/json"));
                // http.addHeader(F("Authorization"), auth_header);
                http.setUserAgent(userAgent);

                // Use whatever method we were passed
                httpResponseCode = http.sendRequest(httpMethodToString(method), payload);

                response = http.getString();

                if (httpResponseCode < HTTP_CODE_OK || httpResponseCode > HTTP_CODE_NO_CONTENT) {
                    Log.error("send_json_str: Send failed (%d): %s. Response:\r\n%s\r\n",
                        httpResponseCode,
                        http.errorToString(httpResponseCode).c_str(),
                        http.getString().c_str());
                    result = sendResult::failure;
                } else {
                    Log.verbose(F("send_json_str: Response:\r\n%s\r\n"), http.getString().c_str());
                    result = sendResult::success;
                }
                http.end();
            } else {
                Log.error(F("send_json_str: Unable to create connection\r\n"));
                result = sendResult::failure;
            }
        }
        // delete client;
    }

    // send_lock = false;
    return result;
}
