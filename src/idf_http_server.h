/**
 * @file idf_http_server.h
 * @brief ESP-IDF HTTP server abstraction
 *
 * This replaces ESPAsyncWebServer with native esp-idf http_server.
 * Handlers run synchronously on the httpd task.
 */

#ifndef IDF_HTTP_SERVER_H
#define IDF_HTTP_SERVER_H

#include <esp_http_server.h>

// Configuration
#define HTTP_SERVER_PORT            80
#define HTTP_MAX_URI_LEN            512
#define HTTP_MAX_RESP_SIZE          4096

/**
 * @brief Initialize and start the HTTP server
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t idf_httpd_start(void);

/**
 * @brief Stop the HTTP server
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t idf_httpd_stop(void);

/**
 * @brief Get the HTTP server handle
 * @return httpd_handle_t or NULL if not started
 */
httpd_handle_t idf_httpd_get_handle(void);

/**
 * @brief Register a URI handler with the server
 * @param uri_handler Pointer to httpd_uri_t structure
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t idf_httpd_register_uri(const httpd_uri_t *uri_handler);

/**
 * @brief Check if server is running
 * @return true if server is active
 */
bool idf_httpd_is_running(void);

#endif // IDF_HTTP_SERVER_H
