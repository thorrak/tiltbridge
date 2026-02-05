/**
 * @file idf_json_utils.h
 * @brief JSON utilities bridging ArduinoJson with ESP-IDF HTTP server
 *
 * Provides functions for parsing JSON request bodies and sending JSON responses
 * using ArduinoJson for serialization.
 */

#ifndef IDF_JSON_UTILS_H
#define IDF_JSON_UTILS_H

#include <esp_http_server.h>
#include <ArduinoJson.h>

// Maximum JSON body size we'll accept
#define JSON_MAX_BODY_SIZE 4096

/**
 * @brief Parse JSON body from HTTP request into ArduinoJson document
 *
 * Reads the request body and deserializes it into the provided JsonDocument.
 * Sets appropriate error responses on failure.
 *
 * @param req HTTP request handle
 * @param doc JsonDocument to populate with parsed data
 * @return ESP_OK on success, ESP_FAIL on error (response already sent)
 */
esp_err_t idf_json_parse_body(httpd_req_t *req, JsonDocument &doc);

/**
 * @brief Send ArduinoJson document as HTTP response
 *
 * Serializes the JsonDocument and sends it with Content-Type: application/json
 *
 * @param req HTTP request handle
 * @param doc JsonDocument to serialize and send
 * @param status HTTP status code (default 200)
 * @return ESP_OK on success, error code on failure
 */
esp_err_t idf_json_send_response(httpd_req_t *req, JsonDocument &doc, int status = 200);

/**
 * @brief Send a simple JSON status response
 *
 * Sends {"status":"ok"} or {"status":"error"} based on success parameter
 *
 * @param req HTTP request handle
 * @param success true for ok status (200), false for error status (400)
 * @return ESP_OK on success, error code on failure
 */
esp_err_t idf_json_send_status(httpd_req_t *req, bool success);

/**
 * @brief Send a JSON error response with message
 *
 * Sends {"error":"message"} with specified HTTP status
 *
 * @param req HTTP request handle
 * @param status HTTP status code
 * @param message Error message
 * @return ESP_OK on success, error code on failure
 */
esp_err_t idf_json_send_error(httpd_req_t *req, int status, const char *message);

/**
 * @brief Set CORS headers on response
 *
 * Adds Access-Control-Allow-Origin and related headers for CORS support
 *
 * @param req HTTP request handle
 * @return ESP_OK on success
 */
esp_err_t idf_json_set_cors_headers(httpd_req_t *req);

#endif // IDF_JSON_UTILS_H
