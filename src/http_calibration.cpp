#include "http_calibration.h"
#include <ArduinoJson.h>
#include <thorlog.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "filesystem.h"
#include "jsonconfig.h"
#include "tilt/tiltHydrometer.h"
#include "JsonKeys.h"

// Buffer size for JSON file operations
#define JSON_FILE_BUFFER_SIZE 2048

/**
 * @brief Read file contents into a dynamically allocated buffer
 * @param filename Path to file
 * @param out_size Output parameter for file size
 * @return Allocated buffer (caller must free) or NULL on error
 */
static char* read_file_to_buffer(const char* filename, size_t* out_size) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size == 0 || size > JSON_FILE_BUFFER_SIZE) {
        fclose(file);
        return NULL;
    }

    // Allocate buffer
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    // Read file
    size_t read = fread(buffer, 1, size, file);
    fclose(file);

    if (read != size) {
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0';
    if (out_size) {
        *out_size = size;
    }
    return buffer;
}

/**
 * @brief Write JSON document to file via buffer
 * @param filename Path to file
 * @param doc JSON document to write
 * @return true on success
 */
static bool write_json_to_file(const char* filename, JsonDocument& doc) {
    // Measure required size
    size_t json_size = measureJson(doc);
    if (json_size == 0 || json_size > JSON_FILE_BUFFER_SIZE) {
        return false;
    }

    // Allocate buffer
    char* buffer = (char*)malloc(json_size + 1);
    if (!buffer) {
        return false;
    }

    // Serialize to buffer
    serializeJson(doc, buffer, json_size + 1);

    // Write to file
    FILE *file = fopen(filename, "w");
    if (!file) {
        free(buffer);
        return false;
    }

    size_t written = fwrite(buffer, 1, json_size, file);
    fclose(file);
    free(buffer);

    return written == json_size;
}

/**
 * @brief Read JSON document from file via buffer
 * @param filename Path to file
 * @param doc JSON document to populate
 * @return DeserializationError
 */
static DeserializationError read_json_from_file(const char* filename, JsonDocument& doc) {
    size_t size;
    char* buffer = read_file_to_buffer(filename, &size);
    if (!buffer) {
        return DeserializationError::EmptyInput;
    }

    DeserializationError error = deserializeJson(doc, buffer);
    free(buffer);
    return error;
}

// Function to handle calibration data points via POST
bool processCalibrationDataPoint(const JsonDocument& json, bool triggerUpstreamUpdate) {
    // Extract required fields
    if (!json["color"].is<uint8_t>() || !json["rawGravity"].is<double>() || !json["actualGravity"].is<double>()) {
        Log.error("Error: Missing required calibration data fields.\r\n");
        return false;
    }

    uint8_t color = json["color"].as<uint8_t>();

    // Validate color
    if (color >= TILT_COLORS) {
        Log.error("Error: Invalid Tilt color: %d.\r\n", color);
        return false;
    }

    double rawGravity = json["rawGravity"].as<double>();
    double actualGravity = json["actualGravity"].as<double>();

    // Create filename for calibration data
    char filename[64];
    snprintf(filename, sizeof(filename), "%s/%d-cal.json", CONFIG_DIR, color);

    // Load existing calibration data or create new document
    JsonDocument calDoc;
    JsonArray dataPoints;

    if (filesystem_exists(filename)) {
        // Load existing file
        DeserializationError error = read_json_from_file(filename, calDoc);
        if (error) {
            Log.error("Error: Failed to parse existing calibration file: %s\r\n", error.c_str());
            // Create new document if parsing fails
            dataPoints = calDoc.to<JsonArray>();
        } else {
            dataPoints = calDoc.as<JsonArray>();
        }
    } else {
        // Create new document
        dataPoints = calDoc.to<JsonArray>();
    }

    // Add new calibration point as array [rawGravity, actualGravity]
    JsonArray newPoint = dataPoints.add<JsonArray>();
    newPoint.add(rawGravity);
    newPoint.add(actualGravity);

    // Save to file
    if (!write_json_to_file(filename, calDoc)) {
        Log.error("Error: Failed to write calibration file.\r\n");
        return false;
    }

    Log.notice("Calibration point saved: color=%d, raw=%.3f, actual=%.3f\r\n",
               color, rawGravity, actualGravity);

    return true;
}

// Function to handle polynomial coefficients via PUT/PATCH
bool processCalibrationCoefficients(const JsonDocument& json, bool triggerUpstreamUpdate) {
    // Extract required fields
    if (!json["color"].is<uint8_t>()) {
        Log.error("Error: Missing color field for calibration coefficients.\r\n");
        return false;
    }

    uint8_t color = json["color"].as<uint8_t>();

    // Validate color
    if (color >= TILT_COLORS) {
        Log.error("Error: Invalid Tilt color: %d.\r\n", color);
        return false;
    }

    // Update coefficients if provided
    bool updated = false;

    if (json["x0"].is<double>()) {
        config.tilt_calibration[color].x0 = json["x0"].as<double>();
        updated = true;
        Log.notice("Updated x0 for color %d: %.6f\r\n", color, config.tilt_calibration[color].x0);
    }

    if (json["x1"].is<double>()) {
        config.tilt_calibration[color].x1 = json["x1"].as<double>();
        updated = true;
        Log.notice("Updated x1 for color %d: %.6f\r\n", color, config.tilt_calibration[color].x1);
    }

    if (json["x2"].is<double>()) {
        config.tilt_calibration[color].x2 = json["x2"].as<double>();
        updated = true;
        Log.notice("Updated x2 for color %d: %.6f\r\n", color, config.tilt_calibration[color].x2);
    }

    if (json["x3"].is<double>()) {
        config.tilt_calibration[color].x3 = json["x3"].as<double>();
        updated = true;
        Log.notice("Updated x3 for color %d: %.6f\r\n", color, config.tilt_calibration[color].x3);
    }

    if (updated) {
        // Save configuration
        if (!config.save()) {
            Log.error("Error: Unable to save calibration coefficients.\r\n");
            return false;
        }

        Log.notice("Calibration coefficients saved for color %d\r\n", color);
    }

    return true;
}

// Function to get calibration points for a specific color
bool getCalibrationPoints(uint8_t color, JsonDocument& doc) {
    if (color >= TILT_COLORS) {
        Log.error("Error: Invalid Tilt color: %d.\r\n", color);
        return false;
    }

    char filename[64];
    snprintf(filename, sizeof(filename), "%s/%d-cal.json", CONFIG_DIR, color);

    if (!filesystem_exists(filename)) {
        // Return empty array if file doesn't exist
        doc.to<JsonArray>();
        return true;
    }

    DeserializationError error = read_json_from_file(filename, doc);
    if (error) {
        Log.error("Error: Failed to parse calibration file: %s\r\n", error.c_str());
        return false;
    }

    return true;
}

// Function to clear calibration points for a specific color
bool clearCalibrationPoints(uint8_t color) {
    if (color >= TILT_COLORS) {
        Log.error("Error: Invalid Tilt color: %d.\r\n", color);
        return false;
    }

    char filename[64];
    snprintf(filename, sizeof(filename), "%s/%d-cal.json", CONFIG_DIR, color);

    if (filesystem_exists(filename)) {
        if (!(remove(filename) == 0)) {
            Log.error("Error: Failed to delete calibration file.\r\n");
            return false;
        }
        Log.notice("Calibration points cleared for color %d\r\n", color);
    }

    return true;
}

// Function to delete individual calibration data point by raw gravity
bool deleteCalibrationPoint(uint8_t color, double rawGravity) {
    if (color >= TILT_COLORS) {
        Log.error("Error: Invalid Tilt color: %d.\r\n", color);
        return false;
    }

    char filename[64];
    snprintf(filename, sizeof(filename), "%s/%d-cal.json", CONFIG_DIR, color);

    if (!filesystem_exists(filename)) {
        Log.error("Error: No calibration file exists for color %d.\r\n", color);
        return false;
    }

    // Load existing calibration data
    JsonDocument calDoc;
    DeserializationError error = read_json_from_file(filename, calDoc);
    if (error) {
        Log.error("Error: Failed to parse calibration file: %s\r\n", error.c_str());
        return false;
    }

    JsonArray dataPoints = calDoc.as<JsonArray>();

    // Find and remove the point with matching raw gravity
    bool found = false;
    for (int i = dataPoints.size() - 1; i >= 0; i--) {
        JsonArray point = dataPoints[i];
        if (point.size() >= 2 && point[0].is<double>() &&
            abs(point[0].as<double>() - rawGravity) < 0.001) {
            dataPoints.remove(i);
            found = true;
            break;
        }
    }

    if (!found) {
        Log.error("Error: Calibration point with raw gravity %.3f not found.\r\n", rawGravity);
        return false;
    }

    // Save updated data back to file
    if (!write_json_to_file(filename, calDoc)) {
        Log.error("Error: Failed to write calibration file.\r\n");
        return false;
    }

    Log.notice("Calibration point deleted: color=%d, raw=%.3f\r\n", color, rawGravity);
    return true;
}

// Function to handle calibration data points via DELETE
bool processCalibrationDataDelete(const JsonDocument& json, bool triggerUpstreamUpdate) {
    // Extract required fields
    if (!json["color"].is<uint8_t>()) {
        Log.error("Error: Missing color field for calibration deletion.\r\n");
        return false;
    }

    uint8_t color = json["color"].as<uint8_t>();

    // Check if rawGravity is provided for individual point deletion
    if (json["rawGravity"].is<double>()) {
        double rawGravity = json["rawGravity"].as<double>();
        return deleteCalibrationPoint(color, rawGravity);
    } else {
        // No rawGravity provided, clear all points
        return clearCalibrationPoints(color);
    }
}
