/**
 * @file idf_json_utils.cpp
 * @brief JSON utilities implementation
 */

#include "idf_json_utils.h"

#include <esp_log.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "idf_json";

esp_err_t idf_json_parse_body(httpd_req_t *req, JsonDocument &doc) {
    // Check content length
    size_t content_len = req->content_len;
    if (content_len == 0) {
        ESP_LOGW(TAG, "Empty request body");
        idf_json_send_error(req, 400, "Empty request body");
        return ESP_FAIL;
    }

    if (content_len > JSON_MAX_BODY_SIZE) {
        ESP_LOGW(TAG, "Request body too large: %d > %d", content_len, JSON_MAX_BODY_SIZE);
        idf_json_send_error(req, 413, "Request body too large");
        return ESP_FAIL;
    }

    // Allocate buffer for body
    char *body = (char *)malloc(content_len + 1);
    if (body == NULL) {
        ESP_LOGE(TAG, "Failed to allocate body buffer");
        idf_json_send_error(req, 500, "Memory allocation failed");
        return ESP_FAIL;
    }

    // Read the body
    int received = 0;
    while (received < content_len) {
        int ret = httpd_req_recv(req, body + received, content_len - received);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                // Retry on timeout
                continue;
            }
            ESP_LOGE(TAG, "Failed to receive body: %d", ret);
            free(body);
            idf_json_send_error(req, 500, "Failed to read request body");
            return ESP_FAIL;
        }
        received += ret;
    }
    body[content_len] = '\0';

    ESP_LOGD(TAG, "Received JSON body: %s", body);

    // Parse JSON
    DeserializationError error = deserializeJson(doc, body);
    free(body);

    if (error) {
        ESP_LOGW(TAG, "JSON parse error: %s", error.c_str());
        idf_json_send_error(req, 400, "Invalid JSON");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t idf_json_send_response(httpd_req_t *req, JsonDocument &doc, int status) {
    // Set content type
    httpd_resp_set_type(req, "application/json");

    // Set status if not 200
    if (status != 200) {
        char status_str[16];
        snprintf(status_str, sizeof(status_str), "%d", status);
        httpd_resp_set_status(req, status_str);
    }

    // Measure JSON size
    size_t json_len = measureJson(doc);
    if (json_len == 0) {
        // Empty document, send empty object
        return httpd_resp_sendstr(req, "{}");
    }

    // Allocate buffer
    char *json_str = (char *)malloc(json_len + 1);
    if (json_str == NULL) {
        ESP_LOGE(TAG, "Failed to allocate JSON buffer");
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory error");
    }

    // Serialize
    serializeJson(doc, json_str, json_len + 1);

    ESP_LOGD(TAG, "Sending JSON response: %s", json_str);

    // Send response
    esp_err_t ret = httpd_resp_sendstr(req, json_str);
    free(json_str);

    return ret;
}

esp_err_t idf_json_send_status(httpd_req_t *req, bool success) {
    httpd_resp_set_type(req, "application/json");

    if (success) {
        return httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    } else {
        httpd_resp_set_status(req, "400");
        return httpd_resp_sendstr(req, "{\"status\":\"error\"}");
    }
}

esp_err_t idf_json_send_error(httpd_req_t *req, int status, const char *message) {
    httpd_resp_set_type(req, "application/json");

    // Set HTTP status
    char status_str[32];
    switch (status) {
        case 400:
            httpd_resp_set_status(req, "400 Bad Request");
            break;
        case 404:
            httpd_resp_set_status(req, "404 Not Found");
            break;
        case 413:
            httpd_resp_set_status(req, "413 Payload Too Large");
            break;
        case 500:
            httpd_resp_set_status(req, "500 Internal Server Error");
            break;
        case 503:
            httpd_resp_set_status(req, "503 Service Unavailable");
            break;
        default:
            snprintf(status_str, sizeof(status_str), "%d", status);
            httpd_resp_set_status(req, status_str);
            break;
    }

    // Build error JSON
    char error_json[256];
    snprintf(error_json, sizeof(error_json), "{\"error\":\"%s\"}", message);

    return httpd_resp_sendstr(req, error_json);
}

esp_err_t idf_json_set_cors_headers(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    return ESP_OK;
}
