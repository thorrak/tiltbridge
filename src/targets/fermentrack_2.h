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


namespace FermentrackAPIEndpoints {
    // constexpr auto fullConfig = "/api/tiltbridge/device/fullconfig/";
    constexpr auto registerDevice = "/api/tiltbridge/device/register/";
    // constexpr auto status = "/api/tiltbridge/device/status/";
    // constexpr auto messages = "/api/tiltbridge/device/messages/";
}; // namespace UpstreamAPIEndpoints

namespace Fermentrack2SettingsKeys {
// constexpr auto upstreamHost = "upstreamHost";
// constexpr auto upstreamPort = "upstreamPort";
constexpr auto deviceID = "deviceID";
constexpr auto username = "username";
constexpr auto apiKey = "apiKey";
constexpr auto fermentrackRegistrationError = "fermentrackRegistrationError";
// constexpr auto messageID = "messageID";
constexpr auto guid = "guid";
// constexpr auto firmwareVersion = "fwVersion";
// constexpr auto firmwareRelease = "fwRelease";
// constexpr auto firmwareRevision = "fwRevision";
// constexpr auto firmwareTag = "fwTag";

constexpr auto hardware = "hardware";
constexpr auto version = "version";

}; // namespace UpstreamSettingsKeys



extern fermentrackRegErrorT fermentrackRegistrationError;  //<! Error code from last upstream registration attempt
