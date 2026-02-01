#include "http_calibration.h"
#include <ArduinoJson.h>
#include <thorlog.h>
#include "filesystem.h"
#include "jsonconfig.h"
#include "tilt/tiltHydrometer.h"
#include "JsonKeys.h"

// Function to handle calibration data points via POST
bool processCalibrationDataPoint(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;
    
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
    char filename[32];
    snprintf(filename, sizeof(filename), "%s/%d-cal.json", CONFIG_DIR, color);
    
    // Load existing calibration data or create new document
    JsonDocument calDoc;
    JsonArray dataPoints;
    
    if (FILESYSTEM.exists(filename)) {
        // Load existing file
        File file = FILESYSTEM.open(filename, "r");
        if (file) {
            DeserializationError error = deserializeJson(calDoc, file);
            file.close();
            
            if (error) {
                Log.error("Error: Failed to parse existing calibration file: %s\r\n", error.c_str());
                // Create new document if parsing fails
                dataPoints = calDoc.to<JsonArray>();
            } else {
                dataPoints = calDoc.as<JsonArray>();
            }
        } else {
            Log.error("Error: Failed to open calibration file for reading.\r\n");
            dataPoints = calDoc.to<JsonArray>();
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
    File file = FILESYSTEM.open(filename, "w");
    if (!file) {
        Log.error("Error: Failed to open calibration file for writing.\r\n");
        return false;
    }
    
    serializeJson(calDoc, file);
    file.close();
    
    Log.notice("Calibration point saved: color=%d, raw=%.3f, actual=%.3f\r\n", 
               color, rawGravity, actualGravity);
    
    return true;
}

// Function to handle polynomial coefficients via PUT/PATCH
bool processCalibrationCoefficients(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;
    
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
    
    char filename[32];
    snprintf(filename, sizeof(filename), "%s/%d-cal.json", CONFIG_DIR, color);
    
    if (!FILESYSTEM.exists(filename)) {
        // Return empty array if file doesn't exist
        doc.to<JsonArray>();
        return true;
    }
    
    File file = FILESYSTEM.open(filename, "r");
    if (!file) {
        Log.error("Error: Failed to open calibration file for reading.\r\n");
        return false;
    }
    
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
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
    
    char filename[32];
    snprintf(filename, sizeof(filename), "%s/%d-cal.json", CONFIG_DIR, color);
    
    if (FILESYSTEM.exists(filename)) {
        if (!FILESYSTEM.remove(filename)) {
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
    
    char filename[32];
    snprintf(filename, sizeof(filename), "%s/%d-cal.json", CONFIG_DIR, color);
    
    if (!FILESYSTEM.exists(filename)) {
        Log.error("Error: No calibration file exists for color %d.\r\n", color);
        return false;
    }
    
    // Load existing calibration data
    JsonDocument calDoc;
    File file = FILESYSTEM.open(filename, "r");
    if (!file) {
        Log.error("Error: Failed to open calibration file for reading.\r\n");
        return false;
    }
    
    DeserializationError error = deserializeJson(calDoc, file);
    file.close();
    
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
    file = FILESYSTEM.open(filename, "w");
    if (!file) {
        Log.error("Error: Failed to open calibration file for writing.\r\n");
        return false;
    }
    
    serializeJson(calDoc, file);
    file.close();
    
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