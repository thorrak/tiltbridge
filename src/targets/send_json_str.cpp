
#include <string.h>
#include <thorlog.h>

#include "send_json_str.h"
#include "version.h"

// I would love this to be constexpr, but that fails to compile for some reason on recent builds for the S3.
// Thanks, Espressif.
#ifndef ESP32S3
constexpr 
#endif
const char* httpMethodToString(httpMethod method) {
    if (method == httpMethod::HTTP_PUT)
        return "PUT";
    else if (method == httpMethod::HTTP_POST)
        return "POST";
    else if (method == httpMethod::HTTP_PATCH)
        return "PATCH";
    else if (method == httpMethod::HTTP_DELETE)
        return "DELETE";
    else if (method == httpMethod::HTTP_GET)
        return "GET";
    return "GET";
}

void get_useragent(char *ua, size_t size) {
    snprintf(ua, size,
        "tiltbridge/%s (commit %s)",
        version(),
        build()
    );
}

sendResult send_json_str(const char *payload, const char *url, httpMethod method) {
    return send_json_str(payload, url, nullptr, 0, method);
}

sendResult send_json_str(const char *payload, const char *url, char *response, size_t response_size, httpMethod method) {
    char userAgent[128];
    int httpResponseCode;
    sendResult result;

    // Initialize response buffer if provided
    if (response != nullptr && response_size > 0) {
        response[0] = '\0';
    }

    if (WiFi.status() != WL_CONNECTED) {
        Log.warning("send_json_str: Wifi not connected, skipping send.\r\n");
        return sendResult::retry;
    }

    get_useragent(userAgent, sizeof(userAgent));

    size_t payload_len = (payload != nullptr) ? strlen(payload) : 0;

    // Log the request appropriately based on whether we have a payload
    if (payload_len > 0) {
        Log.info("send_json_str: Sending %s with payload to %s\r\n", httpMethodToString(method), url);
    } else {
        Log.info("send_json_str: Sending %s to %s\r\n", httpMethodToString(method), url);
    }

    yield();  // Yield before we lock up the radio

    WiFiClient client;
    if(true) {
        {
            // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClient
            HTTPClient http;

            http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
#ifndef ESP8266
            http.setConnectTimeout(6000);
#endif
            http.setReuse(false);

            if (http.begin(client, url)) {
                // Only add Content-Type header if we have a JSON payload
                if (payload_len > 0) {
                    http.addHeader("Content-Type", "application/json");
                }
                http.setUserAgent(userAgent);

                // Use whatever method we were passed
                // Pass empty string if payload is null
                httpResponseCode = http.sendRequest(httpMethodToString(method), payload != nullptr ? payload : "");

                // Use a local buffer for the response - we copy from getString() immediately
                // to avoid keeping the Arduino String around
                char httpResponseBuf[2048];
                strlcpy(httpResponseBuf, http.getString().c_str(), sizeof(httpResponseBuf));

                // Copy response to caller's buffer if provided
                if (response != nullptr && response_size > 0) {
                    strlcpy(response, httpResponseBuf, response_size);
                }

                if (httpResponseCode < HTTP_CODE_OK || httpResponseCode > HTTP_CODE_NO_CONTENT) {
                    Log.error("send_json_str: Send failed (%d): %s. Response:\r\n%s\r\n",
                        httpResponseCode,
                        HTTPClient::errorToString(httpResponseCode),
                        httpResponseBuf);
                    result = sendResult::failure;
                } else {
                    Log.verbose("send_json_str: Response:\r\n%s\r\n", httpResponseBuf);
                    result = sendResult::success;
                }
                http.end();
            } else {
                Log.error("send_json_str: Unable to create connection\r\n");
                result = sendResult::failure;
            }
        }
    }

    return result;
}
