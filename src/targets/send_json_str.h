#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <HTTPClient.h>
#endif

// #include <WiFiMulti.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <Ticker.h>
#include <ArduinoJson.hpp>




enum class sendResult {
    success,
    failure,
    retry
};

enum class httpMethod {
    HTTP_PUT,
    HTTP_POST,
    HTTP_PATCH,
    HTTP_GET,
    HTTP_DELETE
};


sendResult send_json_str(String &payload, const char *url, httpMethod method);
sendResult send_json_str(String &payload, const char *url, String &response, httpMethod method);
