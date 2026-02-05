/**
 * @file idf_static_files.h
 * @brief Static file serving for ESP-IDF HTTP server with LittleFS support
 *
 * Provides handlers for serving static files from LittleFS filesystem,
 * with support for gzip pre-compression and cache control headers.
 */

#ifndef IDF_STATIC_FILES_H
#define IDF_STATIC_FILES_H

#include <esp_http_server.h>

// Cache control max-age in seconds (10 minutes)
#define STATIC_CACHE_MAX_AGE 600

// Maximum file path length
#define STATIC_MAX_PATH_LEN 128

// Chunk size for file streaming
#define STATIC_FILE_CHUNK_SIZE 1024

/**
 * @brief Serve a static file from LittleFS
 *
 * Handles gzip compression detection, content-type detection,
 * and cache control headers.
 *
 * @param req HTTP request handle
 * @param file_path Path to file in filesystem (without leading /)
 * @return ESP_OK on success, ESP_FAIL if file not found
 */
esp_err_t idf_static_serve_file(httpd_req_t *req, const char *file_path);

/**
 * @brief Generic static file handler
 *
 * Uses the request URI to determine which file to serve.
 * Can be registered as a handler for specific URIs.
 *
 * @param req HTTP request handle
 * @return ESP_OK on success, error code on failure
 */
esp_err_t idf_static_handler(httpd_req_t *req);

/**
 * @brief SPA (Single Page Application) handler
 *
 * Always serves index.html regardless of the request URI.
 * Used for Vue.js client-side routing.
 *
 * @param req HTTP request handle
 * @return ESP_OK on success, error code on failure
 */
esp_err_t idf_static_spa_handler(httpd_req_t *req);

/**
 * @brief 404 Not Found handler
 *
 * Attempts to serve the requested file, falls back to 404 response.
 *
 * @param req HTTP request handle
 * @return ESP_OK (always, sends 404 if file not found)
 */
esp_err_t idf_static_not_found_handler(httpd_req_t *req);

/**
 * @brief Get content type for a file based on extension
 *
 * @param filename Filename or path with extension
 * @return Content-Type string (e.g., "text/html")
 */
const char *idf_static_get_content_type(const char *filename);

/**
 * @brief Register static file routes with the HTTP server
 *
 * Registers the root handler and common static file patterns.
 *
 * @return ESP_OK on success
 */
esp_err_t idf_static_register_handlers(void);

/**
 * @brief Register SPA routes for Vue.js
 *
 * Registers all known Vue.js routes to serve index.html
 *
 * @return ESP_OK on success
 */
esp_err_t idf_static_register_spa_routes(void);

/**
 * @brief Register catch-all handler for static files
 *
 * This must be called LAST after all other handlers are registered
 * so that specific routes take precedence over the wildcard.
 *
 * @return ESP_OK on success
 */
esp_err_t idf_static_register_catchall(void);

#endif // IDF_STATIC_FILES_H
