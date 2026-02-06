
#include <stdlib.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_http_client.h>
#include <esp_netif.h>
#include <esp_crt_bundle.h>
#include <esp_tls.h>

#include <thorlog.h>

#include "send_json_str.h"
#include "url_utils.h"
#include "version.h"

// Structure to hold response data during HTTP event handling
struct HttpResponseContext {
    char* buffer;
    size_t buffer_size;
    size_t bytes_received;
};

// HTTP event handler to capture response body
static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    HttpResponseContext* ctx = (HttpResponseContext*)evt->user_data;

    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (ctx != nullptr && ctx->buffer != nullptr) {
                // Calculate how much we can copy
                size_t space_left = ctx->buffer_size - ctx->bytes_received - 1; // -1 for null terminator
                size_t copy_len = (evt->data_len < space_left) ? evt->data_len : space_left;

                if (copy_len > 0) {
                    memcpy(ctx->buffer + ctx->bytes_received, evt->data, copy_len);
                    ctx->bytes_received += copy_len;
                    ctx->buffer[ctx->bytes_received] = '\0';
                }
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

// HTTP status codes
#define HTTP_CODE_OK 200
#define HTTP_CODE_NO_CONTENT 204

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

static esp_http_client_method_t httpMethodToEspMethod(httpMethod method) {
    switch (method) {
        case httpMethod::HTTP_PUT:
            return HTTP_METHOD_PUT;
        case httpMethod::HTTP_POST:
            return HTTP_METHOD_POST;
        case httpMethod::HTTP_PATCH:
            return HTTP_METHOD_PATCH;
        case httpMethod::HTTP_DELETE:
            return HTTP_METHOD_DELETE;
        case httpMethod::HTTP_GET:
        default:
            return HTTP_METHOD_GET;
    }
}

static bool is_wifi_connected() {
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif == nullptr) {
        return false;
    }
    return esp_netif_is_netif_up(netif);
}

void get_useragent(char *ua, size_t size) {
    snprintf(ua, size,
        "tiltbridge/%s (commit %s)",
        version(),
        build()
    );
}

// =============================================================================
// URL Resolution Helper
// =============================================================================
// Builds a URL with resolved IP address for mDNS hostnames. This is needed
// because esp_http_client doesn't support mDNS resolution directly.
static bool buildResolvedUrl(const char* originalUrl, char* resolvedUrl, size_t bufferSize) {
    ParsedUrl parsed;
    if (!parseUrl(originalUrl, &parsed)) {
        // If parsing fails, use original URL and let esp_http_client handle it
        strlcpy(resolvedUrl, originalUrl, bufferSize);
        return true;
    }

    // Check if host needs mDNS resolution
    if (isMDNS(parsed.host)) {
        char resolvedIp[16];
        if (!resolveHostToString(parsed.host, resolvedIp, sizeof(resolvedIp))) {
            Log.error("buildResolvedUrl: Failed to resolve mDNS host: %s\r\n", parsed.host);
            return false;
        }

        // Rebuild URL with resolved IP
        // Format: scheme://resolvedIp:port/path?query
        int written = snprintf(resolvedUrl, bufferSize, "%s://%s",
                              parsed.scheme, resolvedIp);

        // Add port if non-default
        bool isDefaultPort = (strcmp(parsed.scheme, "http") == 0 && parsed.port == 80) ||
                            (strcmp(parsed.scheme, "https") == 0 && parsed.port == 443);
        if (!isDefaultPort && parsed.port > 0) {
            written += snprintf(resolvedUrl + written, bufferSize - written, ":%d", parsed.port);
        }

        // Add path
        if (strlen(parsed.path) > 0) {
            written += snprintf(resolvedUrl + written, bufferSize - written, "%s", parsed.path);
        } else {
            written += snprintf(resolvedUrl + written, bufferSize - written, "/");
        }

        // Add query if present
        if (strlen(parsed.query) > 0) {
            written += snprintf(resolvedUrl + written, bufferSize - written, "?%s", parsed.query);
        }

        Log.verbose("buildResolvedUrl: Resolved mDNS: %s -> %s\r\n", originalUrl, resolvedUrl);
    } else {
        // No resolution needed
        strlcpy(resolvedUrl, originalUrl, bufferSize);
    }

    return true;
}

// =============================================================================
// Unified HTTP Request Implementation
// =============================================================================
sendResult http_request(const char* url, httpMethod method, const char* payload, char* response, size_t response_size, const HttpRequestOptions& options)
{
    char userAgent[128];
    int httpResponseCode;
    sendResult result;

    // Initialize response buffer if provided
    if (response != nullptr && response_size > 0) {
        response[0] = '\0';
    }

    if (!is_wifi_connected()) {
        Log.warning("http_request: WiFi not connected, skipping send.\r\n");
        return sendResult::retry;
    }

    // Resolve mDNS hostnames
    char resolvedUrl[512];
    if (!buildResolvedUrl(url, resolvedUrl, sizeof(resolvedUrl))) {
        return sendResult::failure;
    }

    get_useragent(userAgent, sizeof(userAgent));

    size_t payload_len = (payload != nullptr) ? strlen(payload) : 0;

    // Log the request appropriately based on whether we have a payload
    if (payload_len > 0) {
        Log.info("http_request: Sending %s with payload to %s\r\n", httpMethodToString(method), resolvedUrl);
    } else {
        Log.info("http_request: Sending %s to %s\r\n", httpMethodToString(method), resolvedUrl);
    }

    vTaskDelay(pdMS_TO_TICKS(1));  // Yield before we lock up the radio

    // Allocate response buffer on heap to avoid stack overflow
    // (loopTask has limited stack space)
    constexpr size_t RESPONSE_BUF_SIZE = 2048;
    char* httpResponseBuf = (char*)malloc(RESPONSE_BUF_SIZE);
    if (httpResponseBuf == nullptr) {
        Log.error("http_request: Failed to allocate response buffer\r\n");
        return sendResult::failure;
    }
    httpResponseBuf[0] = '\0';
    HttpResponseContext responseCtx = {
        .buffer = httpResponseBuf,
        .buffer_size = RESPONSE_BUF_SIZE,
        .bytes_received = 0
    };

    // Configure the HTTP client
    esp_http_client_config_t config = {};
    config.url = resolvedUrl;
    config.timeout_ms = options.timeoutMs;
    config.disable_auto_redirect = false;
    config.event_handler = http_event_handler;
    config.user_data = &responseCtx;

    // HTTPS configuration - skip certificate validation if requested
    // This matches the Arduino WiFiClientSecure.setInsecure() behavior
    if (options.skipCertValidation) {
        config.skip_cert_common_name_check = true;
        // Don't attach the certificate bundle for insecure mode
        config.crt_bundle_attach = nullptr;
    } else {
        // Use the built-in CA certificate bundle for validation
        config.crt_bundle_attach = esp_crt_bundle_attach;
    }

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == nullptr) {
        Log.error("http_request: Unable to create connection\r\n");
        free(httpResponseBuf);
        return sendResult::failure;
    }

    // Set the HTTP method
    esp_http_client_set_method(client, httpMethodToEspMethod(method));

    // Set headers
    if (payload_len > 0 && options.contentType != nullptr) {
        esp_http_client_set_header(client, "Content-Type", options.contentType);
    }
    esp_http_client_set_header(client, "User-Agent", userAgent);

    if (options.acceptHeader != nullptr) {
        esp_http_client_set_header(client, "Accept", options.acceptHeader);
    }

    // Set authorization header if provided (e.g., for InfluxDB)
    if (options.authHeader != nullptr) {
        esp_http_client_set_header(client, "Authorization", options.authHeader);
    }

    // Set payload if provided
    if (payload != nullptr && payload_len > 0) {
        esp_http_client_set_post_field(client, payload, payload_len);
    }

    // Perform the request
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        httpResponseCode = esp_http_client_get_status_code(client);

        // Response body was captured by the event handler into httpResponseBuf
        // Copy response to caller's buffer if provided
        if (response != nullptr && response_size > 0) {
            strlcpy(response, httpResponseBuf, response_size);
        }

        // Check HTTP response code
        if (httpResponseCode < HTTP_CODE_OK || httpResponseCode > HTTP_CODE_NO_CONTENT) {
            Log.error("http_request: Send failed (%d). Response:\r\n%s\r\n", httpResponseCode, httpResponseBuf);
            result = sendResult::failure;
        } else {
            // Body check if requested
            if (options.bodyCheck != nullptr && strlen(options.bodyCheck) > 0) {
                if (strstr(httpResponseBuf, options.bodyCheck) == nullptr) {
                    Log.error("http_request: Body check failed. Expected '%s' in response: %s\r\n", options.bodyCheck, httpResponseBuf);
                    result = sendResult::failure;
                } else {
                    Log.verbose("http_request: Body check passed.\r\n");
                    result = sendResult::success;
                }
            } else {
                Log.verbose("http_request: Response:\r\n%s\r\n", httpResponseBuf);
                result = sendResult::success;
            }
        }
    } else {
        Log.error("http_request: HTTP request failed: %s\r\n", esp_err_to_name(err));
        result = sendResult::failure;
    }

    esp_http_client_cleanup(client);
    free(httpResponseBuf);
    return result;
}

// Convenience overload for simple requests without response buffer
sendResult http_request(const char* url, httpMethod method, const char* payload) {
    return http_request(url, method, payload, nullptr, 0, HttpRequestOptions{});
}
