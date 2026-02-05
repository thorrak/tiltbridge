#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>

#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>

#include "url_utils.h"
#include <thorlog.h>

#include "filesystem.h"


#include "resetreasons.h"
#include "uptime.h"
#include "version.h"
#include "jsonconfig.h"
#include "tilt/tiltScanner.h"
#include "sendData.h"
#include "JsonKeys.h"

#include "http_server.h"

#include "extended_async_json_handler.h"
#include "targets/fermentrack_2.h"
#include "http_calibration.h"


httpServer http_server;
AsyncWebServer asyncWebServer(WEBPORT);

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



// Settings Page Handlers
bool processTiltBridgeSettingsJson(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;
    bool hostnamechanged = false;


    //////  Generic Settings
    // mDNS ID
    if(json["mdnsID"].is<const char*>()) {
        // Set hostname
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
            // Out of range
            Log.warning("Settings update error, [tzOffset]:(%d) not valid.\r\n", json["tzOffset"].as<int8_t>());
        } else {
            // In range
            config.TZoffset = json["tzOffset"];
            Log.notice("Settings update, [tzOffset]:(%d) applied.\r\n", json["tzOffset"].as<int8_t>());
        }
    } else {
        // Log.warning("Settings update error, [tzOffset]:(%s) (as str) not valid.\r\n", json["tzOffset"].as<const char*>());
        // failCount++;
    }


    // tempUnit
    if(json["tempUnit"].is<const char*>()) {
        if(strcmp(json["tempUnit"].as<const char*>(), "C") != 0 &&  strcmp(json["tempUnit"].as<const char*>(), "F") != 0) {
            // Not C/F
            Log.warning("Settings update error, [tempUnit]:(%s) not valid.\r\n", json["tempUnit"].as<const char*>());
        } else {
            // Is C/F
            strlcpy(config.tempUnit, json["tempUnit"].as<const char*>(), 2);
            Log.notice("Settings update, [tempUnit]:(%s) applied.\r\n", json["tempUnit"].as<const char*>());
        }
    } else {
        // Log.warning("Settings update error, [tempUnit]:(%s) not valid.\r\n", json["tempUnit"].as<const char*>());
        // failCount++;
    }

    // smoothFactor
    if(json["smoothFactor"].is<uint8_t>()) {
        if(json["smoothFactor"].as<uint8_t>() < 0 || json["smoothFactor"].as<uint8_t>() > 99) {
            // Out of range
            Log.warning("Settings update error, [smoothFactor]:(%d) not valid.\r\n", json["smoothFactor"].as<uint8_t>());
        } else {
            // In range
            config.smoothFactor = json["smoothFactor"];
            Log.notice("Settings update, [smoothFactor]:(%d) applied.\r\n", json["smoothFactor"].as<uint8_t>());
        }
    } else {
        // Log.warning("Settings update error, [smoothFactor]:(%s) not valid.\r\n", json["smoothFactor"].as<const char*>());
        // failCount++;
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
    } else {
        // Log.warning("Settings update error, [invertTFT]:(%s) not valid.\r\n", json["invertTFT"].as<const char*>());
        // failCount++;
    }


    // Process everything we were passed
    if (failCount) {
        Log.error("Error: Invalid controller configuration.\r\n");
    } else {
        if (config.save()) {
            if (hostnamechanged) {
                // We reset hostname, process
                hostnamechanged = false;
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


bool updateJsonSettingBool(const JsonDocument& json, const char* key, bool& configVar) {
    if (json[key].is<bool>()) {
        configVar = json[key].as<bool>();
        if(json[key].as<bool>())
            Log.notice("Settings update, [%s]:(True) applied.\r\n", key);
        else
            Log.notice("Settings update, [%s]:(False) applied.\r\n", key);
        return true;
    } else {
        // Not a valid bool
        // Log.warning("Settings update error, [%s]:(%s) not valid.\r\n", key, json[key].as<const char*>());
    }
    return false;
}

bool updateJsonSetting(const JsonDocument& json, const char* key, char* configVar, uint16_t maxLen) {
    if (json[key].is<const char*>()) {
        if(strlen(json[key].as<const char*>()) > maxLen) {
            // Too long
            Log.warning("Settings update error, [%s]:(%s) too long.\r\n", key, json[key].as<const char*>());
        } else {
            // Valid string
            strlcpy(configVar, json[key].as<const char*>(), maxLen);
            Log.notice("Settings update, [%s]:(%s) applied.\r\n", key, json[key].as<const char*>());
            return true;
        }
    } else {
        // Not a valid string
        Log.warning("Settings update error, [%s]:(%s) not valid.\r\n", key, json[key].as<const char*>());
    }
    
    return false;
}

bool updateJsonSetting(const JsonDocument& json, const char* key, uint16_t& configVar) {
    if (json[key].is<uint16_t>()) {
        configVar = json[key].as<uint16_t>();
        Log.notice("Settings update, [%s]:(%d) applied.\r\n", key, json[key].as<uint16_t>());
        return true;
    } else {
        // Not a valid uint16_t
        Log.warning("Settings update error, [%s]:(%s) not valid.\r\n", key, json[key].as<const char*>());
    }
    return false;
}

bool processCalibrationSettings(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;

    // Calibration settings
    if(!updateJsonSettingBool(json, CalibrationKeys::applyCalibration, config.applyCalibration))
        failCount++;

    if(!updateJsonSettingBool(json, CalibrationKeys::tempCorrect, config.tempCorrect))
        failCount++;

    // Save
    if(failCount>0) {
        Log.error("Error: Invalid upstream configuration.\r\n");
    } else if (!config.save()) {
        Log.error("Error: Unable to save calibration configuration data.\r\n");
        failCount++;
    }

    return failCount == 0;
}


bool processFermentrackSettings(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;

    bool update_legacy = false;
    bool update_ft2 = false;

    if (json[FermentrackSettings::legacyFermentrackPushEvery].is<uint16_t>()) {
        Log.info("Received legacy fermentrack settings.\r\n");
        update_legacy = true;
        // Legacy Fermentrack settings
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
        // The only settings that could trigger this relate to how to reach Fermentrack 2
        // In every case, trigger a re-registration
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
        // DeviceID and API Key are set when registering with Fermentrack 2 and cannot be edited by the user
    }



    // Save
    if(failCount>0) {
        Log.error("Error: Invalid Fermentrack target configuration.\r\n");
    } else {
        if (!config.save()) {
            Log.error("Error: Unable to save Fermentrack target configuration data.\r\n");
            failCount++;
        } else {
            // Now that we've saved, trigger the send
            if(update_legacy)  // Trigger a send to Legacy Fermentrack/BPR in 3 seconds using the updated URL
                startSendNowTimer(sendNowLegacyFTTimer, "SendLegacyFT", sendNowLegacyFTCallback, 3);
            if(update_ft2)  // Trigger a send to Fermentrack 2 in 5 seconds using the updated URL
                startSendNowTimer(sendNowFTTimer, "SendFT", sendNowFTCallback, 5);
        }
    }

    return failCount == 0;
}


bool processGoogleSheetsSettings(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;


    if(!updateJsonSetting(json, GoogleSheetsSettings::scriptsURL, config.scriptsURL, 256))
        failCount++;
    if(!updateJsonSetting(json, GoogleSheetsSettings::scriptsEmail, config.scriptsEmail, 256))
        failCount++;
    if(strlen(config.scriptsURL) > 26 && strlen(config.scriptsEmail) > 5)  // Trigger a send to Google in 5 seconds using the updated URL
        startSendNowTimer(sendNowGSheetsTimer, "SendGSheets", sendNowGSheetsCallback, 5);

    // Loop through each of the keys associated with the sheet names, and update the relevant config entry
    uint8_t i=0;
    for(const char* sheetKey : tiltColorSuffixes) {
        char full_key[30];
        // full_key = GoogleSheetsSettings::gsheetsPrefix + sheetKey
        snprintf(full_key, 30, "%s%s", GoogleSheetsSettings::gsheetsPrefix, sheetKey);

        if(!updateJsonSetting(json, full_key, config.gsheets_config[i].name, 25))
            failCount++;
        i++;  // Also track index
    }
    
    // Save
    if(failCount>0) {
        Log.error("Error: Invalid Google Sheets configuration.\r\n");
    } else if (!config.save()) {
        Log.error("Error: Unable to save Google Sheets configuration data.\r\n");
        failCount++;
    }

    return failCount == 0;
}


bool processBrewersFriendSettings(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;


    if(!updateJsonSetting(json, BrewersFriendSettings::brewersFriendKey, config.brewersFriendKey, 64))
        failCount++;
    if(strlen(config.brewersFriendKey) > 1)  // Trigger a send to Brewers Friend
        startSendNowTimer(sendNowBrewersFriendTimer, "SendBF", sendNowBrewersFriendCallback, 5);

    
    // Save
    if(failCount>0) {
        Log.error("Error: Invalid Brewer's Friend configuration.\r\n");
    } else if (!config.save()) {
        Log.error("Error: Unable to save Brewer's Friend configuration data.\r\n");
        failCount++;
    }

    return failCount == 0;
}


bool processBrewfatherSettings(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;


    if(!updateJsonSetting(json, BrewfatherSettings::brewfatherKey, config.brewfatherKey, 64))
        failCount++;
    if(strlen(config.brewfatherKey) > 1)  // Trigger a send to Brewfather
        startSendNowTimer(sendNowBrewfatherTimer, "SendBrewfather", sendNowBrewfatherCallback, 5);

    
    // Save
    if(failCount>0) {
        Log.error("Error: Invalid Brewfather configuration.\r\n");
    } else  if (!config.save()) {
        Log.error("Error: Unable to save Brewfather configuration data.\r\n");
        failCount++;
    }

    return failCount == 0;
}


bool processUserTargetSettings(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;


    if(!updateJsonSetting(json, UserTargetSettings::userTargetURL, config.userTargetURL, 128))
        failCount++;
    if(strlen(config.userTargetURL) > 1)  // Trigger a send to the user target
        startSendNowTimer(sendNowUserTargetTimer, "SendUserTarget", sendNowUserTargetCallback, 5);
 
    
    // Save
    if(failCount>0) {
        Log.error("Error: Invalid user target configuration.\r\n");
    } else if (!config.save()) {
        Log.error("Error: Unable to save user target configuration data.\r\n");
        failCount++;
    }

    return failCount == 0;
}


bool processGrainfatherSettings(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;

    // Loop through each of the keys associated with the sheet names, and update the relevant config entry
    uint8_t i=0;
    for(const char* sheetKey : tiltColorSuffixes) {
        char full_key[35];
        // full_key = GrainfatherSettings::grainfatherURLPrefix + sheetKey
        snprintf(full_key, 35, "%s%s", GrainfatherSettings::grainfatherURLPrefix, sheetKey);

        if(!updateJsonSetting(json, full_key, config.grainfatherURL[i].link, 64))
            failCount++;
        i++;  // Also track index
    }

    startSendNowTimer(sendNowGrainfatherTimer, "SendGrainfather", sendNowGrainfatherCallback, 5);  // Always trigger a resend to grainfather

    
    // Save
    if(failCount>0) {
        Log.error("Error: Invalid Grainfather configuration.\r\n");
    } else if (!config.save()) {
        Log.error("Error: Unable to save Grainfather configuration data.\r\n");
        failCount++;
    }

    return failCount == 0;
}


bool processBrewstatusSettings(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;


    if(!updateJsonSetting(json, BrewstatusSettings::brewstatusURL, config.brewstatusURL, 256))
        failCount++;
    if(strlen(config.brewstatusURL) > 11)  // Trigger a send to BrewStatus in 5 seconds using the updated URL
        startSendNowTimer(sendNowBrewStatusTimer, "SendBrewStatus", sendNowBrewStatusCallback, 5);

    if(!updateJsonSetting(json, BrewstatusSettings::brewstatusPushEvery, config.brewstatusPushEvery))
        failCount++;

    // TODO - Add a check for "push every" to make sure it isn't less than a reasonable value


    // Save
    if(failCount>0) {
        Log.error("Error: Invalid Brewstatus configuration.\r\n");
    } else if (!config.save()) {
        Log.error("Error: Unable to save Brewstatus configuration data.\r\n");
        failCount++;
    }

    return failCount == 0;
}


bool processTaplistioSettings(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;


    if(!updateJsonSetting(json, TaplistioSettings::taplistioURL, config.taplistioURL, 256))
        failCount++;
    if(strlen(config.taplistioURL) > 11)  // Trigger a send to TaplistIO in 5 seconds using the updated URL
        startSendNowTimer(sendNowTaplistioTimer, "SendTaplistio", sendNowTaplistioCallback, 5);

    if(!updateJsonSetting(json, TaplistioSettings::taplistioPushEvery, config.taplistioPushEvery))
        failCount++;

    // TODO - Add a check for "push every" to make sure it isn't less than a reasonable value

    // Save
    if(failCount>0) {
        Log.error("Error: Invalid Taplist.io configuration.\r\n");
    } else if (!config.save()) {
        Log.error("Error: Unable to save Taplist.io configuration data.\r\n");
        failCount++;
    }

    return failCount == 0;
}

bool processMqttSettings(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;


    if(!updateJsonSetting(json, MQTTSettings::mqttBrokerHost, config.mqttBrokerHost, sizeof(config.mqttBrokerHost)))
        failCount++;

    if(!updateJsonSetting(json, MQTTSettings::mqttBrokerPort, config.mqttBrokerPort))
        failCount++;

    // TODO - Add a check for "push every" to make sure it isn't less than a reasonable value
    if(!updateJsonSetting(json, MQTTSettings::mqttPushEvery, config.mqttPushEvery))
        failCount++;

    if(!updateJsonSetting(json, MQTTSettings::mqttUsername, config.mqttUsername, sizeof(config.mqttUsername)))
        failCount++;

    if(!updateJsonSetting(json, MQTTSettings::mqttPassword, config.mqttPassword, sizeof(config.mqttPassword)))
        failCount++;

    if(!updateJsonSetting(json, MQTTSettings::mqttTopic, config.mqttTopic, sizeof(config.mqttTopic)))
        failCount++;

    // Trigger a send to MQTT
    http_server.mqtt_init_rqd = true;
    startSendNowTimer(sendNowMqttTimer, "SendMQTT", sendNowMqttCallback, 5);



    // Save
    if(failCount>0) {
        Log.error("Error: Invalid Taplist.io configuration.\r\n");
    } else if (!config.save()) {
        Log.error("Error: Unable to save Taplist.io configuration data.\r\n");
        failCount++;
    }

    return failCount == 0;
}


bool processInfluxdbSettings(const JsonDocument& json, bool triggerUpstreamUpdate) {
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

    if(strlen(config.influxdbURL) > INFLUXDB_MIN_URL_LENGTH)  // Trigger a send to InfluxDB in 5 seconds using the updated settings
        startSendNowTimer(sendNowInfluxdbTimer, "SendInfluxDB", sendNowInfluxdbCallback, 5);

    // Save
    if(failCount>0) {
        Log.error("Error: Invalid InfluxDB configuration.\r\n");
    } else if (!config.save()) {
        Log.error("Error: Unable to save InfluxDB configuration data.\r\n");
        failCount++;
    }

    return failCount == 0;
}


//-----------------------------------------------------------------------------------------

#ifndef DISABLE_OTA_UPDATES
void trigger_OTA(AsyncWebServerRequest *request) {
    server.serveStatic("/updating.htm", FILESYSTEM, "/").setDefaultFile("updating.htm");
    config.update_filesystem = true;
    lcd.display_ota_update_screen();         // Trigger this here while everything else is waiting.
    vTaskDelay(pdMS_TO_TICKS(1000));         // Wait 1 second to let everything send
    tilt_scanner.wait_until_scan_complete(); // Wait for scans to complete (we don't want any tasks running in the background)
    execOTA();                               // Trigger the OTA update
}
#endif

void http_json(JsonDocument &doc) {
    doc = tilt_scanner.tilt_to_json();
}

void settings_json(JsonDocument &doc) {
    doc = config.to_json_external();
}

// About.htm page Handlers
//

void this_version(JsonDocument &doc) {
    doc["version"] = version();
    doc["branch"] = branch();
    doc["build"] = build();
    doc["hardware"] = hardware();
}

void uptime(JsonDocument &doc) {
    const int days = uptimeDays();
    const int hours = uptimeHours();
    const int minutes = uptimeMinutes();
    const int seconds = uptimeSeconds();;
    const int millis = uptimeMillis();

    doc["days"] = days;
    doc["hours"] = hours;
    doc["minutes"] = minutes;;
    doc["seconds"] = seconds;
    doc["millis"] = millis;
}

void heap(JsonDocument &doc) {
    const uint32_t free = ESP.getFreeHeap();
    const uint32_t max = ESP.getMaxAllocHeap();
    const uint8_t frag = 100 - (max * 100) / free;

    doc["free"] = free;
    doc["max"] = max;
    doc["frag"] = frag;
}

void reset_reason(JsonDocument &doc) {
    const int reset = (int)esp_reset_reason();

    doc["reason"] = resetReason[reset];
    doc["description"] = resetDescription[reset];
}

void httpServer::setStaticPages() {

    // Define the base static page handlers
    asyncWebServer.serveStatic("/", FILESYSTEM, "/index.html").setCacheControl("max-age=600");
    asyncWebServer.serveStatic("/index.html", FILESYSTEM, "/index.html").setCacheControl("max-age=600");

    // Define Vue routes
    const char* vueRoutes[] = {
        "/config", 
        "/config/tiltbridge",
        "/target", 
        "/target/fermentrack", 
        "/target/legacy_fermentrack", 
        "/target/gsheets", 
        "/target/brewersfriend",
        "/target/brewfather", 
        "/target/grainfather", 
        "/target/brewstatus", 
        "/target/taplistio",
        "/target/mqtt", 
        "/target/generic",
        "/help", 
        "/about"
    };


    // Serve static pages for Vue routes and their trailing-slash versions
    for (const char* route : vueRoutes) {
        asyncWebServer.serveStatic(route, FILESYSTEM, "/index.html").setCacheControl("max-age=600");

        // Serve the same route with a trailing slash
        String routeWithSlash = String(route) + "/";
        asyncWebServer.serveStatic(routeWithSlash.c_str(), FILESYSTEM, "/index.html").setCacheControl("max-age=600");
    }

    // // Static page handlers
    // web_server->serveStatic("/", FILESYSTEM, "/index.htm", "max-age=600");
    // web_server->serveStatic("/index/", FILESYSTEM, "/index.htm", "max-age=600");
    // web_server->serveStatic("/settings/", FILESYSTEM, "/settings.htm", "max-age=600");
    // web_server->serveStatic("/calibration/", FILESYSTEM, "/calibration.htm", "max-age=600");
    // web_server->serveStatic("/help/", FILESYSTEM, "/help.htm", "max-age=600");
    // web_server->serveStatic("/about/", FILESYSTEM, "/about.htm", "max-age=600");
    // web_server->serveStatic("/controllerrestart/", FILESYSTEM, "/controllerrestart.htm", "max-age=600");
    // web_server->serveStatic("/wifireset/", FILESYSTEM, "/wifireset.htm", "max-age=600");
    // web_server->serveStatic("/factoryreset/", FILESYSTEM, "/factoryreset.htm", "max-age=600");
    // web_server->serveStatic("/gsheets/", FILESYSTEM, "/gsheets.htm", "max-age=600");
    asyncWebServer.serveStatic("/404/", FILESYSTEM, "/404.htm").setCacheControl("max-age=600");
}

void httpServer::setPutPages() {
    struct Endpoint {
        const char* path;
        bool (*handler)(const JsonDocument&, bool);
    };

    const Endpoint endpoints[] = {
        {"/api/settings/controller/", processTiltBridgeSettingsJson},
        {"/api/settings/calibration/", processCalibrationSettings},
        {"/api/settings/fermentrack/", processFermentrackSettings},
        {"/api/settings/googlesheets/", processGoogleSheetsSettings},
        {"/api/settings/brewersfriend/", processBrewersFriendSettings},
        {"/api/settings/brewfather/", processBrewfatherSettings},
        {"/api/settings/grainfather/", processGrainfatherSettings},
        {"/api/settings/usertarget/", processUserTargetSettings},
        {"/api/settings/brewstatus/", processBrewstatusSettings},
        {"/api/settings/taplistio/", processTaplistioSettings},
        {"/api/settings/mqtt/", processMqttSettings},
        {"/api/settings/influxdb/", processInfluxdbSettings},
    };

    for (const auto& endpoint : endpoints) {
        asyncWebServer.addHandler(new PutAsyncCallbackJsonWebHandler(endpoint.path, endpoint.handler));
    }
}

void httpServer::setJsonPages() {
    struct Endpoint {
        const char* path;
        void (*handler)(JsonDocument&);
    };

    const Endpoint endpoints[] = {
        {"/api/json/", http_json},
        {"/api/settings/json/", settings_json},
        {"/api/version/", this_version},
        {"/api/uptime/", uptime},
        {"/api/heap/", heap},
        {"/api/resetreason/", reset_reason},
    };

    for (const auto& endpoint : endpoints) {
        asyncWebServer.addHandler(new GetAsyncCallbackJsonWebHandler(endpoint.path, endpoint.handler));
    }

}

// TODO - Reenable/rebuild setActionPages
// void setActionPages() {
// #ifndef DISABLE_OTA_UPDATES
//     web_server->on("/ota/", HTTP_GET, [&]() {
//         request->send(200, F("text/plain"), F("Ok."));
//         trigger_OTA(request);
//     });
// #endif

//     web_server->on("/resetwifi/", HTTP_GET, [&]() {
//         Log.verbose("Processing /resetwifi/.\r\n");
//         request->send(200, F("text/plain"), F("Ok."));
//         http_server.wifi_reset_requested = true;
//     });

//     web_server->on("/resetapp/", HTTP_GET, [&]() {
//         Log.verbose("Processing /resetapp/.\r\n");
//         request->send(200, F("text/plain"), F("Ok."));
//         http_server.factoryreset_requested = true;
//     });

//     web_server->on("/oktoreset/", HTTP_GET, [&]() {
//         Log.verbose("Processing /oktoreset/.\r\n");
//         request->send(200, F("text/plain"), F("Ok."));
//         http_server.restart_requested = true;
//     });

//     web_server->on("/ping/", HTTP_ANY, [&]() {
//         Log.verbose("Processing /ping/.\r\n");
//         request->send(200, F("text/plain"), F("Ok."));
//     });
// }

void httpServer::init() {
    setStaticPages();
    setPutPages();
    setJsonPages();
    // setActionPages();

    // Calibration endpoints
    asyncWebServer.addHandler(new PostAsyncCallbackJsonWebHandler("/api/calibration/datapoint/", processCalibrationDataPoint));
    asyncWebServer.addHandler(new PutAsyncCallbackJsonWebHandler("/api/calibration/coefficients/", processCalibrationCoefficients));
    asyncWebServer.addHandler(new PostAsyncCallbackJsonWebHandler("/api/calibration/datapoint/delete/", processCalibrationDataDelete));
    
    /*
    // GET handler for calibration data points with query parameter
    asyncWebServer.on("/api/calibration/datapoints/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("color")) {
            request->send(400, "application/json", "{\"error\":\"Missing color parameter\"}");
            return;
        }
        
        String colorStr = request->getParam("color")->value();
        uint8_t color = colorStr.toInt();
        
        if (color >= TILT_COLORS) {
            request->send(400, "application/json", "{\"error\":\"Invalid color parameter\"}");
            return;
        }
        
        AsyncJsonResponse *response = new AsyncJsonResponse();
        JsonDocument doc;
        
        if (getCalibrationPoints(color, doc)) {
            response->getRoot().set(doc);
            response->setLength();
            request->send(response);
        } else {
            request->send(500, "application/json", "{\"error\":\"Failed to retrieve calibration points\"}");
        }
    });*/

    // File not found handler
    asyncWebServer.onNotFound([](AsyncWebServerRequest *request) {
        if (!http_server.handleFileRead(request, request->url())) {
            request->send(404, "text/plain", "Not Found");
        }
    });

    // DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

    asyncWebServer.begin();
    Log.notice("HTTP server started. Open: http://%s.local/ to view application.\r\n", config.mdnsID);
}
