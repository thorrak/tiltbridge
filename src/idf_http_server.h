/**
 * @file idf_http_server.h
 * @brief ESP-IDF HTTP server abstraction with async worker thread pool
 *
 * This replaces ESPAsyncWebServer with native esp-idf http_server.
 * Provides async request handling via a worker thread pool.
 */

#ifndef IDF_HTTP_SERVER_H
#define IDF_HTTP_SERVER_H

#include <esp_http_server.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

// Configuration
#define HTTP_SERVER_PORT            80
#define HTTP_MAX_ASYNC_REQUESTS     5
#define HTTP_WORKER_TASK_STACK_SIZE 4096
#define HTTP_WORKER_TASK_PRIORITY   5
#define HTTP_MAX_URI_LEN            512
#define HTTP_MAX_RESP_SIZE          4096

/**
 * @brief Handler function type for async processing
 *
 * This is the same signature as httpd_uri_t.handler
 */
typedef esp_err_t (*idf_httpd_handler_t)(httpd_req_t *req);

/**
 * @brief Async request structure for worker thread processing
 */
typedef struct {
    httpd_req_t *req;              // Copied request handle
    idf_httpd_handler_t handler;   // Handler function to execute
} httpd_async_req_t;

/**
 * @brief Initialize and start the HTTP server with worker pool
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t idf_httpd_start(void);

/**
 * @brief Stop the HTTP server and cleanup worker pool
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
 * @brief Queue an async request for worker thread processing
 *
 * Use this from within a handler to defer processing to a worker thread.
 * The original handler should return ESP_OK after calling this.
 *
 * @param req The request to queue (will be copied)
 * @param handler The handler function to execute in worker thread
 * @return ESP_OK if queued, ESP_FAIL if queue full (503 sent automatically)
 */
esp_err_t idf_httpd_queue_async(httpd_req_t *req, idf_httpd_handler_t handler);

/**
 * @brief Check if server is running
 * @return true if server is active
 */
bool idf_httpd_is_running(void);

#endif // IDF_HTTP_SERVER_H
