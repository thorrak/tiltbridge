#pragma once


enum class fermentrackRegErrorT {
    NO_ERROR,                   // 0
    NO_GUID,                    // 1
    NO_USERNAME,                // 2
    INVALID_USER,               // 3
    NO_BREWHOUSE_ON_USER,       // 4
    NO_HARDWARE_TYPE,           // 5
    NO_FIRMWARE_VERSION,        // 6
    INVALID_API_KEY,            // 7
    NOT_ATTEMPTED_REGISTRATION, // 8
    REGISTRATION_ENDPOINT_ERR,  // 9
};

enum class fermentrackStatusErrorT {
    NO_ERROR,                   // 0
    MISSING_DEVICE_ID,          // 1
    MISSING_API_KEY,            // 2
    REGISTRATION_INVALID,       // 3 - Device ID/API key invalid, should delete registration
    RATE_LIMIT,                 // 4 - Rate limit exceeded, status was disregarded (TBD if we end up using this/need to interpret it somehow)
    STATUS_ENDPOINT_ERR,        // 5
};


namespace FermentrackAPIEndpoints {
    // constexpr auto fullConfig = "/api/tiltbridge/device/fullconfig/";
    constexpr auto registerDevice = "/api/tiltbridge/device/register/";
    constexpr auto status = "/api/tiltbridge/device/status/";
    constexpr auto messages = "/api/tiltbridge/device/messages/";
    constexpr auto calibrationPoints = "/api/tiltbridge/device/tilt/cal/points/";
    constexpr auto calibrationPoint = "/api/tiltbridge/device/tilt/cal/point/";
    constexpr auto calibrationCoefficients = "/api/tiltbridge/device/tilt/cal/coefficients/";
}; // namespace UpstreamAPIEndpoints

namespace Fermentrack2SettingsKeys {
constexpr auto deviceID = "deviceID";
constexpr auto username = "username";
constexpr auto apiKey = "apiKey";
constexpr auto fermentrackRegistrationError = "fermentrackRegistrationError";
// constexpr auto messageID = "messageID";
constexpr auto guid = "guid";
constexpr auto hardware = "hardware";
constexpr auto version = "version";
}; // namespace UpstreamSettingsKeys


class FermentrackMessageFlags {
    public:
    bool hasMessages = false;          //<! Flag to indicate that there are messages to be retrieved
    bool pendingResetConnection = false;  //<! Flag to indicate that the connection should be reset
    bool pendingRestartDevice = false;     //<! Flag to indicate that the device should be restarted
    bool pendingSyncCalibration[8] = {false, false, false, false, false, false, false, false};   //<! Flags to indicate that calibration should be synced from Fermentrack for each color
};


extern fermentrackRegErrorT fermentrackRegistrationError;  //<! Error code from last registration attempt
extern fermentrackStatusErrorT fermentrackStatusError;  //<! Error code from last status attempt
extern FermentrackMessageFlags fermentrackMessageFlags;  //<! Flags for messages to be sent to Fermentrack
