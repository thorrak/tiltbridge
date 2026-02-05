/**
 * @file idf_http_server.cpp
 * @brief ESP-IDF HTTP server implementation with async worker thread pool
 */

#include "idf_http_server.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include <string.h>

static const char *TAG = "idf_httpd";

// Server state
static httpd_handle_t s_server = NULL;
static bool s_running = false;

// Worker thread pool state
static QueueHandle_t s_request_queue = NULL;
static SemaphoreHandle_t s_worker_ready_count = NULL;
static TaskHandle_t s_worker_tasks[HTTP_MAX_ASYNC_REQUESTS] = {NULL};

/**
 * @brief Worker task that processes queued async requests
 */
static void worker_task(void *arg) {
    while (true) {
        // Signal that this worker is available
        xSemaphoreGive(s_worker_ready_count);

        // Wait for a request
        httpd_async_req_t async_req;
        if (xQueueReceive(s_request_queue, &async_req, portMAX_DELAY) == pdTRUE) {
            ESP_LOGD(TAG, "Worker processing async request");

            // Execute the handler
            if (async_req.handler && async_req.req) {
                async_req.handler(async_req.req);
            }

            // Complete the async request (frees the copied request)
            if (async_req.req) {
                httpd_req_async_handler_complete(async_req.req);
            }
        }
    }
}

/**
 * @brief Start worker thread pool
 */
static esp_err_t start_workers(void) {
    // Create request queue
    s_request_queue = xQueueCreate(HTTP_MAX_ASYNC_REQUESTS, sizeof(httpd_async_req_t));
    if (s_request_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create request queue");
        return ESP_FAIL;
    }

    // Create counting semaphore for worker availability
    s_worker_ready_count = xSemaphoreCreateCounting(HTTP_MAX_ASYNC_REQUESTS, 0);
    if (s_worker_ready_count == NULL) {
        ESP_LOGE(TAG, "Failed to create worker semaphore");
        vQueueDelete(s_request_queue);
        s_request_queue = NULL;
        return ESP_FAIL;
    }

    // Create worker tasks
    for (int i = 0; i < HTTP_MAX_ASYNC_REQUESTS; i++) {
        char task_name[16];
        snprintf(task_name, sizeof(task_name), "httpd_worker_%d", i);

        BaseType_t result = xTaskCreate(
            worker_task,
            task_name,
            HTTP_WORKER_TASK_STACK_SIZE,
            NULL,
            HTTP_WORKER_TASK_PRIORITY,
            &s_worker_tasks[i]
        );

        if (result != pdPASS) {
            ESP_LOGE(TAG, "Failed to create worker task %d", i);
            // Clean up already created tasks
            for (int j = 0; j < i; j++) {
                if (s_worker_tasks[j]) {
                    vTaskDelete(s_worker_tasks[j]);
                    s_worker_tasks[j] = NULL;
                }
            }
            vSemaphoreDelete(s_worker_ready_count);
            vQueueDelete(s_request_queue);
            s_worker_ready_count = NULL;
            s_request_queue = NULL;
            return ESP_FAIL;
        }
    }

    ESP_LOGI(TAG, "Started %d worker tasks", HTTP_MAX_ASYNC_REQUESTS);
    return ESP_OK;
}

/**
 * @brief Stop worker thread pool
 */
static void stop_workers(void) {
    // Delete worker tasks
    for (int i = 0; i < HTTP_MAX_ASYNC_REQUESTS; i++) {
        if (s_worker_tasks[i]) {
            vTaskDelete(s_worker_tasks[i]);
            s_worker_tasks[i] = NULL;
        }
    }

    // Delete semaphore
    if (s_worker_ready_count) {
        vSemaphoreDelete(s_worker_ready_count);
        s_worker_ready_count = NULL;
    }

    // Delete queue
    if (s_request_queue) {
        vQueueDelete(s_request_queue);
        s_request_queue = NULL;
    }

    ESP_LOGI(TAG, "Stopped worker tasks");
}

esp_err_t idf_httpd_start(void) {
    if (s_running) {
        ESP_LOGW(TAG, "HTTP server already running");
        return ESP_OK;
    }

    // Start worker pool first
    esp_err_t ret = start_workers();
    if (ret != ESP_OK) {
        return ret;
    }

    // Configure HTTP server
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = HTTP_SERVER_PORT;
    config.lru_purge_enable = true;
    config.max_uri_handlers = 64;
    config.max_resp_headers = 8;
    config.uri_match_fn = httpd_uri_match_wildcard;

    // Need extra sockets for async requests + at least one for sync
    config.max_open_sockets = HTTP_MAX_ASYNC_REQUESTS + 2;

    // Start the server
    ret = httpd_start(&s_server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(ret));
        stop_workers();
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

    // Stop workers after server
    stop_workers();

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

esp_err_t idf_httpd_queue_async(httpd_req_t *req, idf_httpd_handler_t handler) {
    if (!s_request_queue || !s_worker_ready_count) {
        ESP_LOGE(TAG, "Worker pool not initialized");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Server error");
        return ESP_FAIL;
    }

    // Check if a worker is available (non-blocking)
    if (xSemaphoreTake(s_worker_ready_count, 0) != pdTRUE) {
        // No workers available, return 503 Service Unavailable
        ESP_LOGW(TAG, "No workers available, returning 503");
        httpd_resp_set_status(req, "503 Service Unavailable");
        httpd_resp_sendstr(req, "Server busy");
        return ESP_FAIL;
    }

    // Create async request copy
    httpd_async_req_t async_req;
    async_req.handler = handler;

    esp_err_t ret = httpd_req_async_handler_begin(req, &async_req.req);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to begin async handler: %s", esp_err_to_name(ret));
        // Give back the semaphore since we didn't use the worker
        xSemaphoreGive(s_worker_ready_count);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Server error");
        return ret;
    }

    // Queue the request
    if (xQueueSend(s_request_queue, &async_req, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to queue async request");
        httpd_req_async_handler_complete(async_req.req);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Server error");
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Queued async request");
    return ESP_OK;
}

bool idf_httpd_is_running(void) {
    return s_running;
}
