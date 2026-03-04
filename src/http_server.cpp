#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>

#include <ArduinoJson.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <esp_system.h>

#include "url_utils.h"
#include "thorlog.h"

#include "resetreasons.h"
#include "uptime.h"
#include "version.h"
#include "jsonconfig.h"
#include "tilt/tiltScanner.h"
#include "sendData.h"
#include "JsonKeys.h"

#include <esp_wifi_manager.h>

#include "http_server.h"
#include "idf_http_server.h"
#include "idf_json_utils.h"
#include "idf_static_files.h"
#include "http_calibration.h"
#include "targets/fermentrack_2.h"

static const char *TAG = "http_server";

httpServer http_server;

// Helper function to clear name reset flag without requiring http_server.h include
// This avoids ESPAsyncWebServer/ESP-IDF http_parser.h enum conflicts
void http_server_clear_name_reset() {
    http_server.name_reset_requested = false;
}

// Timer handles for triggering immediate sends from HTTP configuration updates
static TimerHandle_t sendNowLegacyFTTimer = nullptr;
static TimerHandle_t sendNowFTTimer = nullptr;
static TimerHandle_t sendNowGSheetsTimer = nullptr;
static TimerHandle_t sendNowBrewersFriendTimer = nullptr;
static TimerHandle_t sendNowBrewfatherTimer = nullptr;
static TimerHandle_t sendNowUserTargetTimer = nullptr;
static TimerHandle_t sendNowGrainfatherTimer = nullptr;
static TimerHandle_t sendNowBrewStatusTimer = nullptr;
static TimerHandle_t sendNowTaplistioTimer = nullptr;
static TimerHandle_t sendNowMqttTimer = nullptr;
static TimerHandle_t sendNowInfluxdbTimer = nullptr;

// Timer callbacks for send-now triggers
static void sendNowLegacyFTCallback(TimerHandle_t xTimer) { data_sender.send_legacy_fermentrack = true; }
static void sendNowFTCallback(TimerHandle_t xTimer) { data_sender.send_fermentrack = true; }
static void sendNowGSheetsCallback(TimerHandle_t xTimer) { data_sender.send_gSheets = true; }
static void sendNowBrewersFriendCallback(TimerHandle_t xTimer) { data_sender.send_brewersFriend = true; }
static void sendNowBrewfatherCallback(TimerHandle_t xTimer) { data_sender.send_brewfather = true; }
static void sendNowUserTargetCallback(TimerHandle_t xTimer) { data_sender.send_userTarget = true; }
static void sendNowGrainfatherCallback(TimerHandle_t xTimer) { data_sender.send_grainfather = true; }
static void sendNowBrewStatusCallback(TimerHandle_t xTimer) { data_sender.send_brewStatus = true; }
static void sendNowTaplistioCallback(TimerHandle_t xTimer) { data_sender.send_taplistio = true; }
static void sendNowMqttCallback(TimerHandle_t xTimer) { data_sender.send_mqtt = true; }
static void sendNowInfluxdbCallback(TimerHandle_t xTimer) { data_sender.send_influxdb = true; }

// Helper to start a send-now timer (creates if needed, then starts)
static void startSendNowTimer(TimerHandle_t& timer, const char* name, TimerCallbackFunction_t callback, uint32_t delaySecs) {
    if (timer == nullptr) {
        timer = xTimerCreate(name, pdMS_TO_TICKS(delaySecs * 1000), pdFALSE, nullptr, callback);
    } else {
        xTimerChangePeriod(timer, pdMS_TO_TICKS(delaySecs * 1000), 0);
    }
    if (timer != nullptr) {
        xTimerStart(timer, 0);
    }
}


//=============================================================================
// JSON Response Generators (GET handlers)
//=============================================================================

static void http_json(JsonDocument &doc) {
    doc = tilt_scanner.tilt_to_json();
}

static void settings_json(JsonDocument &doc) {
    doc = config.to_json_external();
}

static void this_version(JsonDocument &doc) {
    doc["version"] = version();
    doc["branch"] = branch();
    doc["build"] = build();
    doc["hardware"] = hardware();
}

static void uptime_json(JsonDocument &doc) {
    const int days = uptimeDays();
    const int hours = uptimeHours();
    const int minutes = uptimeMinutes();
    const int seconds = uptimeSeconds();
    const int millis = uptimeMillis();

    doc["days"] = days;
    doc["hours"] = hours;
    doc["minutes"] = minutes;
    doc["seconds"] = seconds;
    doc["millis"] = millis;
}

static void heap_json(JsonDocument &doc) {
    const uint32_t free = esp_get_free_heap_size();
    const uint32_t max = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    const uint8_t frag = (free > 0) ? (100 - (max * 100) / free) : 0;

    doc["free"] = free;
    doc["max"] = max;
    doc["frag"] = frag;
}

static void reset_reason_json(JsonDocument &doc) {
    const int reset = (int)esp_reset_reason();

    doc["reason"] = resetReason[reset];
    doc["description"] = resetDescription[reset];
}


//=============================================================================
// Settings Update Handlers (PUT/POST handlers)
//=============================================================================

static bool updateJsonSettingBool(const JsonDocument& json, const char* key, bool& configVar) {
    if (json[key].is<bool>()) {
        configVar = json[key].as<bool>();
        if(json[key].as<bool>())
            Log.notice("Settings update, [%s]:(True) applied.\r\n", key);
        else
            Log.notice("Settings update, [%s]:(False) applied.\r\n", key);
        return true;
    }
    return false;
}

static bool updateJsonSetting(const JsonDocument& json, const char* key, char* configVar, uint16_t maxLen) {
    if (json[key].is<const char*>()) {
        if(strlen(json[key].as<const char*>()) > maxLen) {
            Log.warning("Settings update error, [%s]:(%s) too long.\r\n", key, json[key].as<const char*>());
        } else {
            strlcpy(configVar, json[key].as<const char*>(), maxLen);
            Log.notice("Settings update, [%s]:(%s) applied.\r\n", key, json[key].as<const char*>());
            return true;
        }
    } else {
        Log.warning("Settings update error, [%s]:(%s) not valid.\r\n", key, json[key].as<const char*>());
    }
    return false;
}

static bool updateJsonSetting(const JsonDocument& json, const char* key, uint16_t& configVar) {
    if (json[key].is<uint16_t>()) {
        configVar = json[key].as<uint16_t>();
        Log.notice("Settings update, [%s]:(%d) applied.\r\n", key, json[key].as<uint16_t>());
        return true;
    } else {
        Log.warning("Settings update error, [%s]:(%s) not valid.\r\n", key, json[key].as<const char*>());
    }
    return false;
}

static bool processTiltBridgeSettingsJson(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;
    bool hostnamechanged = false;

    // mDNS ID
    if(json["mdnsID"].is<const char*>()) {
        if (!isValidLabel(json["mdnsID"].as<const char*>())) {
            Log.warning("Settings update error, [mdnsID]:(%s) not valid.\r\n", json["mdnsID"]);
            failCount++;
        } else {
            if (strcmp(config.mdnsID, json["mdnsID"].as<const char*>()) != 0) {
                hostnamechanged = true;
                strlcpy(config.mdnsID, json["mdnsID"].as<const char*>(), 32);
                Log.notice("Settings update, [mdnsID]:(%s) applied.\r\n", json["mdnsID"].as<const char*>());
            } else {
                Log.notice("Settings update, [mdnsID]:(%s) NOT applied - no change.\r\n", json["mdnsID"].as<const char*>());
            }
        }
    }

    // tzOffset
    if(json["tzOffset"].is<int8_t>()) {
        if(json["tzOffset"].as<int8_t>() < -12 || json["tzOffset"].as<int8_t>() > 14) {
            Log.warning("Settings update error, [tzOffset]:(%d) not valid.\r\n", json["tzOffset"].as<int8_t>());
        } else {
            config.TZoffset = json["tzOffset"];
            Log.notice("Settings update, [tzOffset]:(%d) applied.\r\n", json["tzOffset"].as<int8_t>());
        }
    }

    // tempUnit
    if(json["tempUnit"].is<const char*>()) {
        if(strcmp(json["tempUnit"].as<const char*>(), "C") != 0 && strcmp(json["tempUnit"].as<const char*>(), "F") != 0) {
            Log.warning("Settings update error, [tempUnit]:(%s) not valid.\r\n", json["tempUnit"].as<const char*>());
        } else {
            strlcpy(config.tempUnit, json["tempUnit"].as<const char*>(), 2);
            Log.notice("Settings update, [tempUnit]:(%s) applied.\r\n", json["tempUnit"].as<const char*>());
        }
    }

    // smoothFactor
    if(json["smoothFactor"].is<uint8_t>()) {
        if(json["smoothFactor"].as<uint8_t>() < 0 || json["smoothFactor"].as<uint8_t>() > 99) {
            Log.warning("Settings update error, [smoothFactor]:(%d) not valid.\r\n", json["smoothFactor"].as<uint8_t>());
        } else {
            config.smoothFactor = json["smoothFactor"];
            Log.notice("Settings update, [smoothFactor]:(%d) applied.\r\n", json["smoothFactor"].as<uint8_t>());
        }
    }

    // invertTFT
    if(json["invertTFT"].is<bool>()) {
        if(config.invertTFT != json["invertTFT"].as<bool>())
            http_server.lcd_reinit_rqd = true;
        config.invertTFT = json["invertTFT"];
        if(json["invertTFT"].as<bool>())
            Log.notice("Settings update, [invertTFT]:(True) applied.\r\n");
        else
            Log.notice("Settings update, [invertTFT]:(False) applied.\r\n");
    }

    // Process everything we were passed
    if (failCount) {
        Log.error("Error: Invalid controller configuration.\r\n");
    } else {
        if (config.save()) {
            if (hostnamechanged) {
                hostnamechanged = false;
                wifi_manager_set_var("mdns_name", config.mdnsID);
                http_server.name_reset_requested = true;
                Log.notice("Received new mDNSid, queued network reset.\r\n");
            }
        } else {
            Log.error("Error: Unable to save controller configuration data.\r\n");
            failCount++;
        }
    }
    return failCount == 0;
}

static bool processCalibrationSettings(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;

    if(!updateJsonSettingBool(json, CalibrationKeys::applyCalibration, config.applyCalibration))
        failCount++;

    if(!updateJsonSettingBool(json, CalibrationKeys::tempCorrect, config.tempCorrect))
        failCount++;

    if(failCount > 0) {
        Log.error("Error: Invalid upstream configuration.\r\n");
    } else if (!config.save()) {
        Log.error("Error: Unable to save calibration configuration data.\r\n");
        failCount++;
    }

    return failCount == 0;
}

static bool processFermentrackSettings(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;

    bool update_legacy = false;
    bool update_ft2 = false;

    if (json[FermentrackSettings::legacyFermentrackPushEvery].is<uint16_t>()) {
        Log.info("Received legacy fermentrack settings.\r\n");
        update_legacy = true;

        if(!updateJsonSetting(json, FermentrackSettings::legacyFermentrackURL, config.legacyFermentrackURL, 256))
            failCount++;

        if(!updateJsonSetting(json, FermentrackSettings::legacyFermentrackPushEvery, config.legacyFermentrackPushEvery))
            failCount++;
        if(config.legacyFermentrackPushEvery < 30 || config.legacyFermentrackPushEvery > 43200) {
            Log.warning("Settings update error, [legacyFermentrackPushEvery]:(%d) not valid.\r\n", config.legacyFermentrackPushEvery);
            config.legacyFermentrackPushEvery = 30;
            failCount++;
        }
    } else {
        Log.info("Received FT2 settings.\r\n");
        update_ft2 = true;
        config.fermentrackDeviceID[0] = '\0';
        fermentrackRegistrationError = fermentrackRegErrorT::NOT_ATTEMPTED_REGISTRATION;

        if(!updateJsonSetting(json, FermentrackSettings::fermentrackHostname, config.fermentrackHostname, sizeof(config.fermentrackHostname)))
            failCount++;
        if(!updateJsonSetting(json, FermentrackSettings::fermentrackPort, config.fermentrackPort))
            failCount++;
        if(config.fermentrackPort < 10 || config.fermentrackPort > 65535) {
            Log.warning("Settings update error, [fermentrackPort]:(%d) not valid.\r\n", config.fermentrackPort);
            config.fermentrackPort = 80;
            failCount++;
        }
        if(!updateJsonSetting(json, FermentrackSettings::fermentrackUsername, config.fermentrackUsername, sizeof(config.fermentrackUsername)))
            failCount++;
    }

    if(failCount > 0) {
        Log.error("Error: Invalid Fermentrack target configuration.\r\n");
    } else {
        if (!config.save()) {
            Log.error("Error: Unable to save Fermentrack target configuration data.\r\n");
            failCount++;
        } else {
            if(update_legacy)
                startSendNowTimer(sendNowLegacyFTTimer, "SendLegacyFT", sendNowLegacyFTCallback, 3);
            if(update_ft2)
                startSendNowTimer(sendNowFTTimer, "SendFT", sendNowFTCallback, 5);
        }
    }

    return failCount == 0;
}

static bool processGoogleSheetsSettings(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;

    if(!updateJsonSetting(json, GoogleSheetsSettings::scriptsURL, config.scriptsURL, 256))
        failCount++;
    if(!updateJsonSetting(json, GoogleSheetsSettings::scriptsEmail, config.scriptsEmail, 256))
        failCount++;
    if(strlen(config.scriptsURL) > 26 && strlen(config.scriptsEmail) > 5)
        startSendNowTimer(sendNowGSheetsTimer, "SendGSheets", sendNowGSheetsCallback, 5);

    uint8_t i = 0;
    for(const char* sheetKey : tiltColorSuffixes) {
        char full_key[30];
        snprintf(full_key, 30, "%s%s", GoogleSheetsSettings::gsheetsPrefix, sheetKey);

        if(!updateJsonSetting(json, full_key, config.gsheets_config[i].name, 25))
            failCount++;
        i++;
    }

    if(failCount > 0) {
        Log.error("Error: Invalid Google Sheets configuration.\r\n");
    } else if (!config.save()) {
        Log.error("Error: Unable to save Google Sheets configuration data.\r\n");
        failCount++;
    }

    return failCount == 0;
}

static bool processBrewersFriendSettings(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;

    if(!updateJsonSetting(json, BrewersFriendSettings::brewersFriendKey, config.brewersFriendKey, 64))
        failCount++;
    if(strlen(config.brewersFriendKey) > 1)
        startSendNowTimer(sendNowBrewersFriendTimer, "SendBF", sendNowBrewersFriendCallback, 5);

    if(failCount > 0) {
        Log.error("Error: Invalid Brewer's Friend configuration.\r\n");
    } else if (!config.save()) {
        Log.error("Error: Unable to save Brewer's Friend configuration data.\r\n");
        failCount++;
    }

    return failCount == 0;
}

static bool processBrewfatherSettings(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;

    if(!updateJsonSetting(json, BrewfatherSettings::brewfatherKey, config.brewfatherKey, 64))
        failCount++;
    if(strlen(config.brewfatherKey) > 1)
        startSendNowTimer(sendNowBrewfatherTimer, "SendBrewfather", sendNowBrewfatherCallback, 5);

    if(failCount > 0) {
        Log.error("Error: Invalid Brewfather configuration.\r\n");
    } else if (!config.save()) {
        Log.error("Error: Unable to save Brewfather configuration data.\r\n");
        failCount++;
    }

    return failCount == 0;
}

static bool processUserTargetSettings(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;

    if(!updateJsonSetting(json, UserTargetSettings::userTargetURL, config.userTargetURL, 128))
        failCount++;
    if(strlen(config.userTargetURL) > 1)
        startSendNowTimer(sendNowUserTargetTimer, "SendUserTarget", sendNowUserTargetCallback, 5);

    if(failCount > 0) {
        Log.error("Error: Invalid user target configuration.\r\n");
    } else if (!config.save()) {
        Log.error("Error: Unable to save user target configuration data.\r\n");
        failCount++;
    }

    return failCount == 0;
}

static bool processGrainfatherSettings(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;

    uint8_t i = 0;
    for(const char* sheetKey : tiltColorSuffixes) {
        char full_key[35];
        snprintf(full_key, 35, "%s%s", GrainfatherSettings::grainfatherURLPrefix, sheetKey);

        if(!updateJsonSetting(json, full_key, config.grainfatherURL[i].link, 64))
            failCount++;
        i++;
    }

    startSendNowTimer(sendNowGrainfatherTimer, "SendGrainfather", sendNowGrainfatherCallback, 5);

    if(failCount > 0) {
        Log.error("Error: Invalid Grainfather configuration.\r\n");
    } else if (!config.save()) {
        Log.error("Error: Unable to save Grainfather configuration data.\r\n");
        failCount++;
    }

    return failCount == 0;
}

static bool processBrewstatusSettings(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;

    if(!updateJsonSetting(json, BrewstatusSettings::brewstatusURL, config.brewstatusURL, 256))
        failCount++;
    if(strlen(config.brewstatusURL) > 11)
        startSendNowTimer(sendNowBrewStatusTimer, "SendBrewStatus", sendNowBrewStatusCallback, 5);

    if(!updateJsonSetting(json, BrewstatusSettings::brewstatusPushEvery, config.brewstatusPushEvery))
        failCount++;

    if(failCount > 0) {
        Log.error("Error: Invalid Brewstatus configuration.\r\n");
    } else if (!config.save()) {
        Log.error("Error: Unable to save Brewstatus configuration data.\r\n");
        failCount++;
    }

    return failCount == 0;
}

static bool processTaplistioSettings(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;

    if(!updateJsonSetting(json, TaplistioSettings::taplistioURL, config.taplistioURL, 256))
        failCount++;
    if(strlen(config.taplistioURL) > 11)
        startSendNowTimer(sendNowTaplistioTimer, "SendTaplistio", sendNowTaplistioCallback, 5);

    if(!updateJsonSetting(json, TaplistioSettings::taplistioPushEvery, config.taplistioPushEvery))
        failCount++;

    if(failCount > 0) {
        Log.error("Error: Invalid Taplist.io configuration.\r\n");
    } else if (!config.save()) {
        Log.error("Error: Unable to save Taplist.io configuration data.\r\n");
        failCount++;
    }

    return failCount == 0;
}

static bool processMqttSettings(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;

    if(!updateJsonSetting(json, MQTTSettings::mqttBrokerHost, config.mqttBrokerHost, sizeof(config.mqttBrokerHost)))
        failCount++;

    if(!updateJsonSetting(json, MQTTSettings::mqttBrokerPort, config.mqttBrokerPort))
        failCount++;

    if(!updateJsonSetting(json, MQTTSettings::mqttPushEvery, config.mqttPushEvery))
        failCount++;

    if(!updateJsonSetting(json, MQTTSettings::mqttUsername, config.mqttUsername, sizeof(config.mqttUsername)))
        failCount++;

    if(!updateJsonSetting(json, MQTTSettings::mqttPassword, config.mqttPassword, sizeof(config.mqttPassword)))
        failCount++;

    if(!updateJsonSetting(json, MQTTSettings::mqttTopic, config.mqttTopic, sizeof(config.mqttTopic)))
        failCount++;

    http_server.mqtt_init_rqd = true;
    startSendNowTimer(sendNowMqttTimer, "SendMQTT", sendNowMqttCallback, 5);

    if(failCount > 0) {
        Log.error("Error: Invalid MQTT configuration.\r\n");
    } else if (!config.save()) {
        Log.error("Error: Unable to save MQTT configuration data.\r\n");
        failCount++;
    }

    return failCount == 0;
}

static bool processInfluxdbSettings(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;

    if(!updateJsonSetting(json, InfluxDBSettings::influxdbURL, config.influxdbURL, sizeof(config.influxdbURL)))
        failCount++;
    if(!updateJsonSetting(json, InfluxDBSettings::influxdbToken, config.influxdbToken, sizeof(config.influxdbToken)))
        failCount++;
    if(!updateJsonSetting(json, InfluxDBSettings::influxdbOrg, config.influxdbOrg, sizeof(config.influxdbOrg)))
        failCount++;
    if(!updateJsonSetting(json, InfluxDBSettings::influxdbBucket, config.influxdbBucket, sizeof(config.influxdbBucket)))
        failCount++;
    if(!updateJsonSetting(json, InfluxDBSettings::influxdbPushEvery, config.influxdbPushEvery))
        failCount++;

    if(strlen(config.influxdbURL) > INFLUXDB_MIN_URL_LENGTH)
        startSendNowTimer(sendNowInfluxdbTimer, "SendInfluxDB", sendNowInfluxdbCallback, 5);

    if(failCount > 0) {
        Log.error("Error: Invalid InfluxDB configuration.\r\n");
    } else if (!config.save()) {
        Log.error("Error: Unable to save InfluxDB configuration data.\r\n");
        failCount++;
    }

    return failCount == 0;
}


/**
 * @brief Unified target settings dispatcher
 *
 * Detects which target's keys are present in the JSON and delegates
 * to the appropriate process*Settings helper.
 */
static bool processTargetSettings(const JsonDocument& json, bool triggerUpstreamUpdate) {
    // Fermentrack — check for either legacy or FT2 key
    if (json[FermentrackSettings::legacyFermentrackPushEvery].is<uint16_t>() ||
        json[FermentrackSettings::fermentrackHostname].is<const char*>())
        return processFermentrackSettings(json, triggerUpstreamUpdate);

    if (json[GoogleSheetsSettings::scriptsURL].is<const char*>())
        return processGoogleSheetsSettings(json, triggerUpstreamUpdate);

    if (json[BrewersFriendSettings::brewersFriendKey].is<const char*>())
        return processBrewersFriendSettings(json, triggerUpstreamUpdate);

    if (json[BrewfatherSettings::brewfatherKey].is<const char*>())
        return processBrewfatherSettings(json, triggerUpstreamUpdate);

    if (json[UserTargetSettings::userTargetURL].is<const char*>())
        return processUserTargetSettings(json, triggerUpstreamUpdate);

    if (json["grainfatherURL_red"].is<const char*>())
        return processGrainfatherSettings(json, triggerUpstreamUpdate);

    if (json[BrewstatusSettings::brewstatusURL].is<const char*>())
        return processBrewstatusSettings(json, triggerUpstreamUpdate);

    if (json[TaplistioSettings::taplistioURL].is<const char*>())
        return processTaplistioSettings(json, triggerUpstreamUpdate);

    if (json[MQTTSettings::mqttBrokerHost].is<const char*>())
        return processMqttSettings(json, triggerUpstreamUpdate);

    if (json[InfluxDBSettings::influxdbURL].is<const char*>())
        return processInfluxdbSettings(json, triggerUpstreamUpdate);

    Log.warning("No recognized target keys in JSON payload.\r\n");
    return false;
}


//=============================================================================
// HTTP Handler Wrappers (bridge existing functions to httpd_req_handler_t)
//=============================================================================

// Type for GET JSON handlers
typedef void (*json_get_handler_t)(JsonDocument &);

// Type for PUT/POST JSON handlers
typedef bool (*json_put_handler_t)(const JsonDocument &, bool);

/**
 * @brief Generic wrapper for GET JSON endpoints
 *
 * Calls the provided handler function to populate a JsonDocument,
 * then sends it as a JSON response.
 */
static esp_err_t json_get_wrapper(httpd_req_t *req, json_get_handler_t handler) {
    JsonDocument doc;
    handler(doc);
    return idf_json_send_response(req, doc);
}

/**
 * @brief Generic wrapper for PUT/POST JSON endpoints
 *
 * Parses the request body as JSON, calls the handler, and returns status.
 */
static esp_err_t json_put_wrapper(httpd_req_t *req, json_put_handler_t handler) {
    JsonDocument doc;

    if (idf_json_parse_body(req, doc) != ESP_OK) {
        return ESP_OK;  // Error response already sent
    }

    bool success = handler(doc, true);
    return idf_json_send_status(req, success);
}

// Macro to create httpd handler from GET JSON function
#define MAKE_GET_HANDLER(name, func) \
    static esp_err_t name(httpd_req_t *req) { \
        return json_get_wrapper(req, func); \
    }

// Macro to create httpd handler from PUT JSON function
#define MAKE_PUT_HANDLER(name, func) \
    static esp_err_t name(httpd_req_t *req) { \
        return json_put_wrapper(req, func); \
    }

// Generate GET handlers
MAKE_GET_HANDLER(handle_api_json, http_json)
MAKE_GET_HANDLER(handle_api_settings_json, settings_json)
MAKE_GET_HANDLER(handle_api_version, this_version)
MAKE_GET_HANDLER(handle_api_uptime, uptime_json)
MAKE_GET_HANDLER(handle_api_heap, heap_json)
MAKE_GET_HANDLER(handle_api_resetreason, reset_reason_json)

// Generate PUT handlers
MAKE_PUT_HANDLER(handle_settings_controller, processTiltBridgeSettingsJson)
MAKE_PUT_HANDLER(handle_settings_calibration, processCalibrationSettings)
MAKE_PUT_HANDLER(handle_settings_targets, processTargetSettings)

// Calibration POST handlers
MAKE_PUT_HANDLER(handle_calibration_datapoint, processCalibrationDataPoint)
MAKE_PUT_HANDLER(handle_calibration_coefficients, processCalibrationCoefficients)
MAKE_PUT_HANDLER(handle_calibration_delete, processCalibrationDataDelete)

// Action handler — dispatches based on "action" field in JSON body
static esp_err_t handle_action(httpd_req_t *req) {
    JsonDocument doc;

    if (idf_json_parse_body(req, doc) != ESP_OK) {
        return ESP_OK;  // Error response already sent
    }

    const char *action = doc["action"];
    if (!action) {
        return idf_json_send_error(req, 400, "Missing 'action' field");
    }

    if (strcmp(action, "resetWifi") == 0) {
        http_server.wifi_reset_requested = true;
    } else if (strcmp(action, "resetDevice") == 0) {
        http_server.factoryreset_requested = true;
    } else if (strcmp(action, "restartDevice") == 0) {
        http_server.restart_requested = true;
    } else {
        return idf_json_send_error(req, 400, "Unknown action");
    }

    return idf_json_send_status(req, true);
}


//=============================================================================
// httpServer class implementation
//=============================================================================

void httpServer::registerJsonGetHandlers() {
    struct {
        const char *uri;
        esp_err_t (*handler)(httpd_req_t *);
    } get_endpoints[] = {
        {"/api/json/", handle_api_json},
        {"/api/settings/json/", handle_api_settings_json},
        {"/api/version/", handle_api_version},
        {"/api/uptime/", handle_api_uptime},
        {"/api/heap/", handle_api_heap},
        {"/api/resetreason/", handle_api_resetreason},
    };

    for (const auto& endpoint : get_endpoints) {
        httpd_uri_t uri_config = {
            .uri = endpoint.uri,
            .method = HTTP_GET,
            .handler = endpoint.handler,
            .user_ctx = NULL
        };
        idf_httpd_register_uri(&uri_config);
    }

    ESP_LOGI(TAG, "Registered JSON GET handlers");
}

void httpServer::registerJsonPutHandlers() {
    struct {
        const char *uri;
        esp_err_t (*handler)(httpd_req_t *);
    } put_endpoints[] = {
        {"/api/settings/controller/",  handle_settings_controller},
        {"/api/settings/calibration/", handle_settings_calibration},
        {"/api/settings/targets/",     handle_settings_targets},
    };

    for (const auto& endpoint : put_endpoints) {
        httpd_uri_t uri_config = {
            .uri = endpoint.uri,
            .method = HTTP_PUT,
            .handler = endpoint.handler,
            .user_ctx = NULL
        };
        idf_httpd_register_uri(&uri_config);
    }

    ESP_LOGI(TAG, "Registered JSON PUT handlers");
}

void httpServer::registerCalibrationHandlers() {
    // POST: Add calibration data point
    httpd_uri_t datapoint_uri = {
        .uri = "/api/calibration/datapoint/",
        .method = HTTP_POST,
        .handler = handle_calibration_datapoint,
        .user_ctx = NULL
    };
    idf_httpd_register_uri(&datapoint_uri);

    // PUT: Update calibration coefficients
    httpd_uri_t coefficients_uri = {
        .uri = "/api/calibration/coefficients/",
        .method = HTTP_PUT,
        .handler = handle_calibration_coefficients,
        .user_ctx = NULL
    };
    idf_httpd_register_uri(&coefficients_uri);

    // POST: Delete calibration data point
    httpd_uri_t delete_uri = {
        .uri = "/api/calibration/datapoint/delete/",
        .method = HTTP_POST,
        .handler = handle_calibration_delete,
        .user_ctx = NULL
    };
    idf_httpd_register_uri(&delete_uri);

    ESP_LOGI(TAG, "Registered calibration handlers");
}

void httpServer::registerActionHandlers() {
    httpd_uri_t uri_config = {
        .uri = "/api/actions/",
        .method = HTTP_POST,
        .handler = handle_action,
        .user_ctx = NULL
    };
    idf_httpd_register_uri(&uri_config);

    ESP_LOGI(TAG, "Registered action handlers");
}

void httpServer::init() {
    // Start the HTTP server with worker pool
    // esp_err_t ret = idf_httpd_start();
    // if (ret != ESP_OK) {
    //     Log.error("Failed to start HTTP server: %s\r\n", esp_err_to_name(ret));
    //     return;
    // }

    // Register static file and SPA handlers first
    idf_static_register_handlers();
    idf_static_register_spa_routes();

    // Register API handlers
    registerJsonGetHandlers();
    registerJsonPutHandlers();
    registerCalibrationHandlers();
    registerActionHandlers();

    // Register catch-all for static files LAST (so specific routes take precedence)
    idf_static_register_catchall();

    Log.notice("HTTP server started. Open: http://%s.local/ to view application.\r\n", config.mdnsID);
}

void httpServer::stop() {
    idf_httpd_stop();
    Log.notice("HTTP server stopped.\r\n");
}
