#ifndef TILTBRIDGE_CALIBRATION_H
#define TILTBRIDGE_CALIBRATION_H

#include <ArduinoJson.h>

// Function prototypes
bool processCalibrationDataPoint(const JsonDocument& json, bool triggerUpstreamUpdate);
bool processCalibrationCoefficients(const JsonDocument& json, bool triggerUpstreamUpdate);
bool processCalibrationDataDelete(const JsonDocument& json, bool triggerUpstreamUpdate);
bool getCalibrationPoints(uint8_t color, JsonDocument& doc);
bool clearCalibrationPoints(uint8_t color);
bool deleteCalibrationPoint(uint8_t color, double rawGravity);

#endif // TILTBRIDGE_CALIBRATION_H