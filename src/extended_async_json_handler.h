/**
 * @file extended_async_json_handler.h
 * @brief A class for handling JSON requests in AsyncWebServer with a custom handler function.
 */

#ifndef EXTENDED_ASYNC_JSON_HANDLER_H_
#define EXTENDED_ASYNC_JSON_HANDLER_H_

#include <AsyncJson.h>

#if ASYNC_JSON_SUPPORT == 1

/**
 * @class RequestAsyncCallbackJsonWebHandler
 * @brief Template base class for JSON handlers that process requests and return success/failure.
 */
template<WebRequestMethod METHOD>
class RequestAsyncCallbackJsonWebHandler : public AsyncCallbackJsonWebHandler
{
protected:
    bool (*_customHandler)(const JsonDocument &, bool);

public:
    RequestAsyncCallbackJsonWebHandler(
        const char *uri, bool (*customHandler)(const JsonDocument &, bool) = nullptr)
        : AsyncCallbackJsonWebHandler(
              uri, [customHandler](AsyncWebServerRequest *request, JsonVariant &json)
              {
                  if (!customHandler) {
                      request->send(500, "application/json", "{\"error\":\"No handler provided\"}");
                      return;
                  }

                  // Parse the JSON payload
                  JsonDocument doc;
                  doc = json.as<JsonObject>();

                  // Call the handler
                  if (customHandler(doc, true)) {
                      request->send(200, "application/json", "{\"status\":\"ok\"}");
                  } else {
                      request->send(400, "application/json", "{\"status\":\"error\"}");
                  }
              }),
        _customHandler(customHandler)
    {
        setMethod(METHOD);
    }
};

/**
 * @class GetAsyncCallbackJsonWebHandler
 * @brief Extends AsyncCallbackJsonWebHandler to support custom JSON response handling for HTTP GET.
 */
class GetAsyncCallbackJsonWebHandler : public AsyncCallbackJsonWebHandler
{
protected:
    void (*_customHandler)(JsonDocument &);

public:
    GetAsyncCallbackJsonWebHandler(
        const char *uri,
        void (*customHandler)(JsonDocument &) = nullptr)
        : AsyncCallbackJsonWebHandler(
              uri,
              [customHandler](AsyncWebServerRequest *request, JsonVariant &json)
              {
                  if (!customHandler) {
                      request->send(500, "application/json", "{\"error\":\"No handler provided\"}");
                      return;
                  }

                  AsyncJsonResponse *response = new AsyncJsonResponse();
                  {
                      JsonDocument doc;
                      customHandler(doc);

                      // // Print the contents of doc to the serial console
                      // Serial.println(F("Generated JSON:"));
                      // serializeJsonPretty(doc, Serial); // Pretty print for easier reading
                      // Serial.println(); // Add a newline for better formatting

                      response->getRoot().set(doc);
                  }

                  response->setLength();
                  request->send(response);
              }),
          _customHandler(customHandler)
    {
        setMethod(HTTP_GET);
    }
};

// Type aliases for specific HTTP methods
using PostAsyncCallbackJsonWebHandler = RequestAsyncCallbackJsonWebHandler<HTTP_POST>;
using PutAsyncCallbackJsonWebHandler = RequestAsyncCallbackJsonWebHandler<HTTP_PUT>;
using DeleteAsyncCallbackJsonWebHandler = RequestAsyncCallbackJsonWebHandler<HTTP_DELETE>;

#endif // ASYNC_JSON_SUPPORT == 1

#endif // EXTENDED_ASYNC_JSON_HANDLER_H_
