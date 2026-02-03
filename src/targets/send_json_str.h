#ifndef SEND_JSON_STR_H
#define SEND_JSON_STR_H

#include <stddef.h>
#include <esp_http_client.h>
#include <esp_wifi.h>
#include <esp_netif.h>

#include <ArduinoJson.hpp>

// =============================================================================
// Content type constants (shared across HTTP sending functions)
// =============================================================================
constexpr auto content_json = "application/json";
constexpr auto content_x_www_form_urlencoded = "application/x-www-form-urlencoded";
constexpr auto content_text_plain = "text/plain; charset=utf-8";

// =============================================================================
// ESP-IDF INITIALIZATION REQUIREMENTS
// =============================================================================
// When fully converting to ESP-IDF, the following must be called at startup
// BEFORE using any functions in this module:
//
// #include "esp_netif.h"
// #include "esp_event.h"
// #include "esp_wifi.h"
// #include "nvs_flash.h"
//
// void init_networking() {
//     // Initialize NVS (required for WiFi)
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(ret);
//
//     // Initialize TCP/IP network interface
//     ESP_ERROR_CHECK(esp_netif_init());
//
//     // Create default event loop (required for esp_http_client callbacks)
//     ESP_ERROR_CHECK(esp_event_loop_create_default());
//
//     // Create default WiFi station interface
//     esp_netif_create_default_wifi_sta();
//
//     // Initialize WiFi with default config
//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));
//
//     // Configure and connect WiFi (application-specific)
//     // ...
// }
// =============================================================================




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


// =============================================================================
// HTTP Request Options
// =============================================================================
// Options struct for the unified http_request function. Provides extensibility
// while maintaining reasonable defaults for common use cases.
struct HttpRequestOptions {
    const char* contentType = content_json;   // Content-Type header
    bool skipCertValidation = true;           // Skip HTTPS cert validation (like Arduino setInsecure())
    const char* bodyCheck = nullptr;          // String to verify in response body (nullptr = no check)
    int timeoutMs = 6000;                     // Request timeout in milliseconds
    const char* authHeader = nullptr;         // Authorization header value (e.g., "Token xxx")
    const char* acceptHeader = content_json;  // Accept header value
};

// =============================================================================
// Unified HTTP Request Function (Primary API)
// =============================================================================
/**
 * @brief Send an HTTP request with full options support
 *
 * This is the primary unified HTTP sending function that supports:
 * - All HTTP methods (GET, POST, PUT, PATCH, DELETE)
 * - Custom content types
 * - mDNS hostname resolution
 * - HTTPS with optional certificate validation bypass
 * - Response body retrieval
 * - Response body verification
 * - Custom authorization headers
 *
 * @param url The URL to send the request to (supports http://, https://, and .local mDNS)
 * @param method The HTTP method to use
 * @param payload The request body (can be nullptr for GET/DELETE)
 * @param response Buffer to store response body (can be nullptr if not needed)
 * @param response_size Size of the response buffer
 * @param options Request options (content type, cert validation, etc.)
 * @return sendResult::success on success, sendResult::failure on error, sendResult::retry if WiFi disconnected
 */
sendResult http_request(
    const char* url,
    httpMethod method,
    const char* payload,
    char* response,
    size_t response_size,
    const HttpRequestOptions& options = HttpRequestOptions{}
);

// Convenience overload for simple requests without response buffer
sendResult http_request(const char* url, httpMethod method, const char* payload);

#endif // SEND_JSON_STR_H
