/**
 * @file idf_http_server.cpp
 * @brief ESP-IDF HTTP server implementation
 *
 * All registered handlers run synchronously on the httpd task. The previous
 * async worker thread pool was never used (idf_httpd_queue_async had no
 * callers) and cost ~20 KB of permanent task stacks plus extra open sockets,
 * so it was removed to reclaim heap on memory-constrained ESP32 targets.
 */

#include "idf_http_server.h"

#include <esp_log.h>
#include <string.h>

static const char *TAG = "idf_httpd";

// Server state
static httpd_handle_t s_server = NULL;
static bool s_running = false;

esp_err_t idf_httpd_start(void) {
    if (s_running) {
        ESP_LOGW(TAG, "HTTP server already running");
        return ESP_OK;
    }

    // Configure HTTP server
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = HTTP_SERVER_PORT;
    config.lru_purge_enable = true;
    config.max_uri_handlers = 40;
    config.max_resp_headers = 8;
    config.uri_match_fn = httpd_uri_match_wildcard;

    // Handlers run synchronously; a small socket pool is plenty.
    config.max_open_sockets = 4;

    // Start the server
    esp_err_t ret = httpd_start(&s_server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(ret));
        return ret;
    }

    s_running = true;
    ESP_LOGI(TAG, "HTTP server started on port %d", HTTP_SERVER_PORT);
    return ESP_OK;
}

esp_err_t idf_httpd_stop(void) {
    if (!s_running || !s_server) {
        ESP_LOGW(TAG, "HTTP server not running");
        return ESP_OK;
    }

    // Stop the HTTP server
    esp_err_t ret = httpd_stop(s_server);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop HTTP server: %s", esp_err_to_name(ret));
        return ret;
    }

    s_server = NULL;
    s_running = false;

    ESP_LOGI(TAG, "HTTP server stopped");
    return ESP_OK;
}

httpd_handle_t idf_httpd_get_handle(void) {
    return s_server;
}

esp_err_t idf_httpd_register_uri(const httpd_uri_t *uri_handler) {
    if (!s_server) {
        ESP_LOGE(TAG, "Cannot register URI: server not started");
        return ESP_FAIL;
    }

    esp_err_t ret = httpd_register_uri_handler(s_server, uri_handler);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register URI %s: %s", uri_handler->uri, esp_err_to_name(ret));
    } else {
        ESP_LOGD(TAG, "Registered URI: %s", uri_handler->uri);
    }
    return ret;
}

bool idf_httpd_is_running(void) {
    return s_running;
}
