
#include <LCBUrl.h>
#include <ArduinoLog.h>
#include <Ticker.h>
#include <WebServer.h>

#if FILESYSTEM == SPIFFS
#include <SPIFFS.h>
#endif


#include "resetreasons.h"
#include "uptime.h"
#include "version.h"
#include "jsonconfig.h"
#include "tilt/tiltScanner.h"
#include "sendData.h"
#include "JsonKeys.h"

#include "http_server.h"


httpServer http_server;
Ticker sendNowTicker;


void httpServer::genericServeJson(void(*jsonFunc)(DynamicJsonDocument&)) {
    String serializedJson;  // Use String here to prevent stack overflow
    DynamicJsonDocument doc(8192);
    jsonFunc(doc);
    serializeJson(doc, serializedJson);
    doc.clear();
    web_server->send(200, "application/json", serializedJson);
}


// Settings Page Handlers
bool processTiltBridgeSettingsJson(const DynamicJsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;
    bool hostnamechanged = false;


    //////  Generic Settings
    // mDNS ID
    if(json.containsKey("mdnsID")) {
        // Set hostname
        LCBUrl url;
        if (!url.isValidLabel(json["mdnsID"])) {
            Log.warning(F("Settings update error, [mdnsID]:(%s) not valid.\r\n"), json["mdnsID"]);
            failCount++;
        } else {
            if (strcmp(config.mdnsID, json["mdnsID"].as<const char*>()) != 0) {
                hostnamechanged = true;
                strlcpy(config.mdnsID, json["mdnsID"].as<const char*>(), 32);
                Log.notice(F("Settings update, [mdnsID]:(%s) applied.\r\n"), json["mdnsID"].as<const char*>());
            } else {
                Log.notice(F("Settings update, [mdnsID]:(%s) NOT applied - no change.\r\n"), json["mdnsID"].as<const char*>());
            }
        }
    }

    // tzOffset
    if(json.containsKey("tzOffset")) {
        if(json["tzOffset"].is<int8_t>()) {
            if(json["tzOffset"].as<int8_t>() < -12 || json["tzOffset"].as<int8_t>() > 14) {
                // Out of range
                Log.warning(F("Settings update error, [tzOffset]:(%d) not valid.\r\n"), json["tzOffset"].as<int8_t>());
            } else {
                // In range
                config.TZoffset = json["tzOffset"];
                Log.notice(F("Settings update, [tzOffset]:(%d) applied.\r\n"), json["tzOffset"].as<int8_t>());
            }
        } else {
            Log.warning(F("Settings update error, [tzOffset]:(%s) (as str) not valid.\r\n"), json["tzOffset"].as<const char*>());
            failCount++;
        }
    }

    // tempUnit
    if(json.containsKey("tempUnit")) {
        if(json["tempUnit"].is<const char*>()) {
            if(strcmp(json["tempUnit"].as<const char*>(), "C") != 0 &&  strcmp(json["tempUnit"].as<const char*>(), "F") != 0) {
                // Not C/F
                Log.warning(F("Settings update error, [tempUnit]:(%s) not valid.\r\n"), json["tempUnit"].as<const char*>());
            } else {
                // Is C/F
                strlcpy(config.tempUnit, json["tempUnit"].as<const char*>(), 2);
                Log.notice(F("Settings update, [tempUnit]:(%s) applied.\r\n"), json["tempUnit"].as<const char*>());
            }
        } else {
            Log.warning(F("Settings update error, [tempUnit]:(%s) not valid.\r\n"), json["tempUnit"].as<const char*>());
            failCount++;
        }
    }

    // smoothFactor
    if(json.containsKey("smoothFactor")) {
        if(json["smoothFactor"].is<uint8_t>()) {
            if(json["smoothFactor"].as<uint8_t>() < 0 || json["smoothFactor"].as<uint8_t>() > 99) {
                // Out of range
                Log.warning(F("Settings update error, [smoothFactor]:(%d) not valid.\r\n"), json["smoothFactor"].as<uint8_t>());
            } else {
                // In range
                config.smoothFactor = json["smoothFactor"];
                Log.notice(F("Settings update, [smoothFactor]:(%d) applied.\r\n"), json["smoothFactor"].as<uint8_t>());
            }
        } else {
            Log.warning(F("Settings update error, [smoothFactor]:(%s) not valid.\r\n"), json["smoothFactor"].as<const char*>());
            failCount++;
        }
    }

    // invertTFT
    if(json.containsKey("invertTFT")) {
        if(json["invertTFT"].is<bool>()) {
            if(config.invertTFT != json["invertTFT"].as<bool>())
                http_server.lcd_reinit_rqd = true;
            config.invertTFT = json["invertTFT"];
            if(json["invertTFT"].as<bool>())
                Log.notice(F("Settings update, [invertTFT]:(True) applied.\r\n"));
            else
                Log.notice(F("Settings update, [invertTFT]:(False) applied.\r\n"));
        } else {
            Log.warning(F("Settings update error, [invertTFT]:(%s) not valid.\r\n"), json["invertTFT"].as<const char*>());
            failCount++;
        }
    }


    // Process everything we were passed
    if (failCount) {
        Log.error(F("Error: Invalid controller configuration.\r\n"));
    } else {
        if (config.save()) {
            if (hostnamechanged) {
                // We reset hostname, process
                hostnamechanged = false;
                http_server.name_reset_requested = true;
                Log.notice(F("Received new mDNSid, queued network reset.\r\n"));
            }
        } else {
            Log.error(F("Error: Unable to save controller configuration data.\r\n"));
            failCount++;
        }
    }
    return failCount == 0;
}


bool updateJsonSettingBool(const DynamicJsonDocument& json, const char* key, bool& configVar) {
    if(json.containsKey(key)) {
        if (json[key].is<bool>()) {
            configVar = json[key].as<bool>();
            if(json[key].as<bool>())
                Log.notice(F("Settings update, [%s]:(True) applied.\r\n"), key);
            else
                Log.notice(F("Settings update, [%s]:(False) applied.\r\n"), key);
            return true;
        } else {
            // Not a valid bool
            Log.warning(F("Settings update error, [%s]:(%s) not valid.\r\n"), key, json[key].as<const char*>());
        }
    }
    return false;
}

bool updateJsonSetting(const DynamicJsonDocument& json, const char* key, char* configVar, uint16_t maxLen) {
    if(json.containsKey(key)) {
        if (json[key].is<const char*>()) {
            if(strlen(json[key].as<const char*>()) > maxLen) {
                // Too long
                Log.warning(F("Settings update error, [%s]:(%s) too long.\r\n"), key, json[key].as<const char*>());
            } else {
                // Valid string
                strlcpy(configVar, json[key].as<const char*>(), maxLen);
                Log.notice(F("Settings update, [%s]:(%s) applied.\r\n"), key, json[key].as<const char*>());
                return true;
            }
        } else {
            // Not a valid string
            Log.warning(F("Settings update error, [%s]:(%s) not valid.\r\n"), key, json[key].as<const char*>());
        }
    }
    return false;
}

bool updateJsonSetting(const DynamicJsonDocument& json, const char* key, uint16_t& configVar) {
    if(json.containsKey(key)) {
        if (json[key].is<uint16_t>()) {
            configVar = json[key].as<uint16_t>();
            return true;
        } else {
            // Not a valid uint16_t
            Log.warning(F("Settings update error, [%s]:(%s) not valid.\r\n"), key, json[key].as<const char*>());
        }
    }
    return false;
}

bool processCalibrationSettings(const DynamicJsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;
    bool saveSettings = false;

    // Calibration settings
    if(!updateJsonSettingBool(json, CalibrationKeys::applyCalibration, config.applyCalibration))
        failCount++;

    if(!updateJsonSettingBool(json, CalibrationKeys::tempCorrect, config.tempCorrect))
        failCount++;

    // Save
    if(failCount>0) {
        Log.error(F("Error: Invalid upstream configuration.\r\n"));
    } else if (saveSettings) {
        if (!config.save()) {
            Log.error(F("Error: Unable to save calibration configuration data.\r\n"));
            failCount++;
        }
    }

    return failCount == 0;
}


bool processFermentrackSettings(const DynamicJsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;
    bool saveSettings = false;


    if(!updateJsonSetting(json, FermentrackSettings::fermentrackURL, config.fermentrackURL, 256))
        failCount++;

    if(!updateJsonSetting(json, FermentrackSettings::fermentrackPushEvery, config.fermentrackPushEvery))
        failCount++;
    if(config.fermentrackPushEvery < 30 || config.fermentrackPushEvery > 43200) {
        Log.warning(F("Settings update error, [fermentrackPushEvery]:(%d) not valid.\r\n"), config.fermentrackPushEvery);
        config.fermentrackPushEvery = 30;
        failCount++;
    }



    // Save
    if(failCount>0) {
        Log.error(F("Error: Invalid Fermentrack target configuration.\r\n"));
    } else if (saveSettings) {
        if (!config.save()) {
            Log.error(F("Error: Unable to save Fermentrack target configuration data.\r\n"));
            failCount++;
        } else {
            // Now that we've saved, trigger the send
            if(strlen(config.fermentrackURL) > 11)  // Trigger a send to Fermentrack/BPR in 5 seconds using the updated URL
                sendNowTicker.once(5, [](){data_sender.send_fermentrack = true;});
        }
    }

    return failCount == 0;
}


bool processGoogleSheetsSettings(const DynamicJsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;
    bool saveSettings = false;


    if(!updateJsonSetting(json, GoogleSheetsSettings::scriptsURL, config.scriptsURL, 256))
        failCount++;
    if(!updateJsonSetting(json, GoogleSheetsSettings::scriptsEmail, config.scriptsEmail, 256))
        failCount++;
    if(strlen(config.scriptsURL) > 26 && strlen(config.scriptsEmail) > 5)  // Trigger a send to Google in 5 seconds using the updated URL
        sendNowTicker.once(5, [](){data_sender.send_gSheets = true;});

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
        Log.error(F("Error: Invalid Google Sheets configuration.\r\n"));
    } else if (saveSettings) {
        if (!config.save()) {
            Log.error(F("Error: Unable to save Google Sheets configuration data.\r\n"));
            failCount++;
        }
    }

    return failCount == 0;
}


bool processBrewersFriendSettings(const DynamicJsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;
    bool saveSettings = false;


    if(!updateJsonSetting(json, BrewersFriendSettings::brewersFriendKey, config.brewersFriendKey, 64))
        failCount++;
    if(strlen(config.brewersFriendKey) > 1)  // Trigger a send to Brewers Friend
        sendNowTicker.once(5, [](){data_sender.send_brewersFriend = true;});

    
    // Save
    if(failCount>0) {
        Log.error(F("Error: Invalid Brewer's Friend configuration.\r\n"));
    } else if (saveSettings) {
        if (!config.save()) {
            Log.error(F("Error: Unable to save Brewer's Friend configuration data.\r\n"));
            failCount++;
        }
    }

    return failCount == 0;
}


bool processBrewfatherSettings(const DynamicJsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;
    bool saveSettings = false;


    if(!updateJsonSetting(json, BrewfatherSettings::brewfatherKey, config.brewfatherKey, 64))
        failCount++;
    if(strlen(config.brewfatherKey) > 1)  // Trigger a send to Brewfather
        sendNowTicker.once(5, [](){data_sender.send_brewfather = true;});

    
    // Save
    if(failCount>0) {
        Log.error(F("Error: Invalid Brewfather configuration.\r\n"));
    } else if (saveSettings) {
        if (!config.save()) {
            Log.error(F("Error: Unable to save Brewfather configuration data.\r\n"));
            failCount++;
        }
    }

    return failCount == 0;
}


bool processUserTargetSettings(const DynamicJsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;
    bool saveSettings = false;


    if(!updateJsonSetting(json, UserTargetSettings::userTargetURL, config.userTargetURL, 128))
        failCount++;
    if(strlen(config.userTargetURL) > 1)  // Trigger a send to the user target
        sendNowTicker.once(5, [](){data_sender.send_userTarget = true;});
 
    
    // Save
    if(failCount>0) {
        Log.error(F("Error: Invalid user target configuration.\r\n"));
    } else if (saveSettings) {
        if (!config.save()) {
            Log.error(F("Error: Unable to save user target configuration data.\r\n"));
            failCount++;
        }
    }

    return failCount == 0;
}


bool processGrainfatherSettings(const DynamicJsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;
    bool saveSettings = false;

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

    sendNowTicker.once(5, [](){data_sender.send_grainfather = true;});  // Always trigger a resend to grainfather

    
    // Save
    if(failCount>0) {
        Log.error(F("Error: Invalid Grainfather configuration.\r\n"));
    } else if (saveSettings) {
        if (!config.save()) {
            Log.error(F("Error: Unable to save Grainfather configuration data.\r\n"));
            failCount++;
        }
    }

    return failCount == 0;
}


bool processBrewstatusSettings(const DynamicJsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;
    bool saveSettings = false;


    if(!updateJsonSetting(json, BrewstatusSettings::brewstatusURL, config.brewstatusURL, 256))
        failCount++;
    if(strlen(config.brewstatusURL) > 11)  // Trigger a send to BrewStatus in 5 seconds using the updated URL
        sendNowTicker.once(5, [](){data_sender.send_brewStatus = true;});

    if(!updateJsonSetting(json, BrewstatusSettings::brewstatusPushEvery, config.brewstatusPushEvery))
        failCount++;

    // TODO - Add a check for "push every" to make sure it isn't less than a reasonable value


    // Save
    if(failCount>0) {
        Log.error(F("Error: Invalid Brewstatus configuration.\r\n"));
    } else if (saveSettings) {
        if (!config.save()) {
            Log.error(F("Error: Unable to save Brewstatus configuration data.\r\n"));
            failCount++;
        }
    }

    return failCount == 0;
}


bool processTaplistioSettings(const DynamicJsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;
    bool saveSettings = false;


    if(!updateJsonSetting(json, TaplistioSettings::taplistioURL, config.taplistioURL, 256))
        failCount++;
    if(strlen(config.taplistioURL) > 11)  // Trigger a send to TaplistIO in 5 seconds using the updated URL
        sendNowTicker.once(5, [](){data_sender.send_taplistio = true;});

    if(!updateJsonSetting(json, TaplistioSettings::taplistioPushEvery, config.taplistioPushEvery))
        failCount++;

    // TODO - Add a check for "push every" to make sure it isn't less than a reasonable value

    // Save
    if(failCount>0) {
        Log.error(F("Error: Invalid Taplist.io configuration.\r\n"));
    } else if (saveSettings) {
        if (!config.save()) {
            Log.error(F("Error: Unable to save Taplist.io configuration data.\r\n"));
            failCount++;
        }
    }

    return failCount == 0;
}

bool processMqttSettings(const DynamicJsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;
    bool saveSettings = false;


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
    sendNowTicker.once(5, [](){data_sender.send_taplistio = true;});



    // Save
    if(failCount>0) {
        Log.error(F("Error: Invalid Taplist.io configuration.\r\n"));
    } else if (saveSettings) {
        if (!config.save()) {
            Log.error(F("Error: Unable to save Taplist.io configuration data.\r\n"));
            failCount++;
        }
    }

    return failCount == 0;
}


// TODO - Reenable/rebuild processCalibration
// // we don't need to do much input checking on the calibration data as we are
// // looking at numbers generated by the javascript and not a human
// void processCalibration(AsyncWebServerRequest *request) {
//     int tilt_color_no = TILT_NONE;
//     int degree = 1;
//     double x0 = 0.0;
//     double x1 = 1.0;
//     double x2 = 0.0;
//     double x3 = 0.0;

//     if (request->hasArg("clearTiltColor")) {
//         // Reset to the defaults
//         tilt_color_no = atoi(request->arg("clearTiltColor").c_str());
//     } else if (request->hasArg("updateTiltColor")) {
//         // Process the settings given
//         tilt_color_no = atoi(request->arg("updateTiltColor").c_str());
//         degree = atoi(request->arg("degree").c_str());
//         if (degree == 1) {
//             Log.verbose(F("Processing linear equation..."));
//             x0 = strtod(request->arg("linearFitx0").c_str(), nullptr);
//             x1 = strtod(request->arg("linearFitx1").c_str(), nullptr);
//             x2 = x3 = 0.0;
//         } else if (degree == 2) {
//             Log.verbose(F("Processing quadratic equation..."));
//             x0 = strtod(request->arg("quadraticFitx0").c_str(), nullptr);
//             x1 = strtod(request->arg("quadraticFitx1").c_str(), nullptr);
//             x2 = strtod(request->arg("quadraticFitx2").c_str(), nullptr);
//             x3 = 0.0;
//         } else if (degree == 3) {
//             Log.verbose(F("Processing cubic equation..."));
//             x0 = strtod(request->arg("cubicFitx0").c_str(), nullptr);
//             x1 = strtod(request->arg("cubicFitx1").c_str(), nullptr);
//             x2 = strtod(request->arg("cubicFitx2").c_str(), nullptr);
//             x3 = strtod(request->arg("cubicFitx3").c_str(), nullptr);
//         } else {
//             // Invalid degree - Reset to defaults
//             Log.verbose(F("Received invalid degree %i\r\n"), degree);
//             processCalibrationError(request);
//         }
//     }

//     if(0 <= tilt_color_no && tilt_color_no < TILT_COLORS) {
//         Log.verbose(F("Saved\r\n"));
//         config.tilt_calibration[tilt_color_no].degree = degree;
//         config.tilt_calibration[tilt_color_no].x0 = x0;
//         config.tilt_calibration[tilt_color_no].x1 = x1;
//         config.tilt_calibration[tilt_color_no].x2 = x2;
//         config.tilt_calibration[tilt_color_no].x3 = x3;
//     } else {
//         Log.verbose(F("Failed\r\n"));
//         processCalibrationError(request);
//     }

//     redirectToCalibration(request);
// }


void httpServer::processJsonRequest(const char* uri, bool (*handler)(const DynamicJsonDocument& json, bool triggerUpstreamUpdate)) {
    // Handler for configuration options
    char message[200] = "";
    uint16_t status_code = 200;
    StaticJsonDocument<200> response;
    Log.verbose(F("Processing %s\r\n"), uri);

    DynamicJsonDocument json(8096);
    DeserializationError error = deserializeJson(json, web_server->arg("plain"));
    if (error) {
        Log.error(F("Error parsing JSON: %s\r\n"), error.c_str());
        response["message"] = "Unable to parse JSON";
        status_code = 400;
    } else {
        if(handler(json, true)) {  // Apply the handler to the data (and trigger an upstream update)
            response["message"] = "Update processed successfully";
        } else {
            response["message"] = "Unable to process update";
            status_code = 400;
        }    
    }

    serializeJson(response, message);
    web_server->send(status_code, "application/json", message);
    
}


//-----------------------------------------------------------------------------------------

#ifndef DISABLE_OTA_UPDATES
void trigger_OTA(AsyncWebServerRequest *request) {
    server.serveStatic("/updating.htm", FILESYSTEM, "/").setDefaultFile("updating.htm");
    config.update_spiffs = true;
    lcd.display_ota_update_screen();         // Trigger this here while everything else is waiting.
    delay(1000);                             // Wait 1 second to let everything send
    tilt_scanner.wait_until_scan_complete(); // Wait for scans to complete (we don't want any tasks running in the background)
    execOTA();                               // Trigger the OTA update
}
#endif

void http_json(DynamicJsonDocument &doc) {
    tilt_scanner.tilt_to_json(doc, false);
}

void settings_json(DynamicJsonDocument &doc) {
    doc = config.to_json_external();
}

// About.htm page Handlers
//

void this_version(DynamicJsonDocument &doc) {
    doc["version"] = version();
    doc["branch"] = branch();
    doc["build"] = build();
}

void uptime(DynamicJsonDocument &doc) {
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

void heap(DynamicJsonDocument &doc) {
    const uint32_t free = ESP.getFreeHeap();
    const uint32_t max = ESP.getMaxAllocHeap();
    const uint8_t frag = 100 - (max * 100) / free;

    doc["free"] = free;
    doc["max"] = max;
    doc["frag"] = frag;
}

void reset_reason(DynamicJsonDocument &doc) {
    const int reset = (int)esp_reset_reason();

    doc["reason"] = resetReason[reset];
    doc["description"] = resetDescription[reset];
}

void httpServer::setStaticPages() {

    web_server->serveStatic("/", FILESYSTEM, "/index.htm", "max-age=600");
    web_server->serveStatic("/index.htm", FILESYSTEM, "/index.htm", "max-age=600");
    web_server->serveStatic("/index.html", FILESYSTEM, "/index.htm", "max-age=600");

    // Static page handlers
    web_server->serveStatic("/", FILESYSTEM, "/index.htm", "max-age=600");
    web_server->serveStatic("/index/", FILESYSTEM, "/index.htm", "max-age=600");
    web_server->serveStatic("/settings/", FILESYSTEM, "/settings.htm", "max-age=600");
    web_server->serveStatic("/calibration/", FILESYSTEM, "/calibration.htm", "max-age=600");
    web_server->serveStatic("/help/", FILESYSTEM, "/help.htm", "max-age=600");
    web_server->serveStatic("/about/", FILESYSTEM, "/about.htm", "max-age=600");
    web_server->serveStatic("/controllerrestart/", FILESYSTEM, "/controllerrestart.htm", "max-age=600");
    web_server->serveStatic("/wifireset/", FILESYSTEM, "/wifireset.htm", "max-age=600");
    web_server->serveStatic("/factoryreset/", FILESYSTEM, "/factoryreset.htm", "max-age=600");
    web_server->serveStatic("/gsheets/", FILESYSTEM, "/gsheets.htm", "max-age=600");
    web_server->serveStatic("/404/", FILESYSTEM, "/404.htm", "max-age=600");
}

void httpServer::setPutPages() {
    // Settings Page Handlers

    web_server->on("/api/settings/controller/", HTTP_PUT, [&]() {
        processJsonRequest("/api/settings/controller/", &processTiltBridgeSettingsJson);
    });

    web_server->on("/api/settings/calibration/", HTTP_PUT, [&]() {
        processJsonRequest("/api/settings/calibration/", &processCalibrationSettings);
    });

    // TODO - Rename/combine these paths
    web_server->on("/api/settings/fermentrack/", HTTP_PUT, [&]() {
        processJsonRequest("/api/settings/fermentrack/", &processFermentrackSettings);
    });

    web_server->on("/api/settings/googlesheets/", HTTP_PUT, [&]() {
        processJsonRequest("/api/settings/googlesheets/", &processGoogleSheetsSettings);
    });

    web_server->on("/api/settings/brewersfriend/", HTTP_PUT, [&]() {
        processJsonRequest("/api/settings/brewersfriend/", &processBrewersFriendSettings);
    });

    web_server->on("/api/settings/brewfather/", HTTP_PUT, [&]() {
        processJsonRequest("/api/settings/brewfather/", &processBrewfatherSettings);
    });

    web_server->on("/api/settings/grainfather/", HTTP_PUT, [&]() {
        processJsonRequest("/api/settings/grainfather/", &processGrainfatherSettings);
    });

    
    web_server->on("/api/settings/usertarget/", HTTP_PUT, [&]() {
        processJsonRequest("/api/settings/usertarget/", &processUserTargetSettings);
    });

    web_server->on("/api/settings/brewstatus/", HTTP_PUT, [&]() {
        processJsonRequest("/api/settings/brewstatus/", &processBrewstatusSettings);
    });

    web_server->on("/api/settings/taplistio/", HTTP_PUT, [&]() {
        processJsonRequest("/api/settings/taplistio/", &processTaplistioSettings);
    });

    web_server->on("/api/settings/mqtt/", HTTP_PUT, [&]() {
        processJsonRequest("/api/settings/mqtt/", &processMqttSettings);
    });
}

void httpServer::setJsonPages() {
    // TODO - Change these to /api/ endpoints
    // Tilt JSON
    web_server->on("/json/", HTTP_GET, [&]() {
        genericServeJson(&http_json);
    });

    // Settings JSON
    web_server->on("/settings/json/", HTTP_GET, [&]() {
        genericServeJson(&settings_json);
    });

    // About Page JSON
    web_server->on("/api/version/", HTTP_GET, [&]() {
        genericServeJson(&this_version);
    });
    web_server->on("/api/uptime/", HTTP_GET, [&]() {
        genericServeJson(&uptime);
    });
    web_server->on("/api/heap/", HTTP_GET, [&]() {
        genericServeJson(&heap);
    });
    web_server->on("/api/resetreason/", HTTP_GET, [&]() {
        genericServeJson(&reset_reason);
    });
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
//         Log.verbose(F("Processing /resetwifi/.\r\n"));
//         request->send(200, F("text/plain"), F("Ok."));
//         http_server.wifi_reset_requested = true;
//     });

//     web_server->on("/resetapp/", HTTP_GET, [&]() {
//         Log.verbose(F("Processing /resetapp/.\r\n"));
//         request->send(200, F("text/plain"), F("Ok."));
//         http_server.factoryreset_requested = true;
//     });

//     web_server->on("/oktoreset/", HTTP_GET, [&]() {
//         Log.verbose(F("Processing /oktoreset/.\r\n"));
//         request->send(200, F("text/plain"), F("Ok."));
//         http_server.restart_requested = true;
//     });

//     web_server->on("/ping/", HTTP_ANY, [&]() {
//         Log.verbose(F("Processing /ping/.\r\n"));
//         request->send(200, F("text/plain"), F("Ok."));
//     });
// }

void httpServer::init() {
    web_server = new WebServer(WEBPORT);
    setStaticPages();
    setPutPages();
    setJsonPages();
    // setActionPages();

    // TODO - Reenable/rebuild processCalibration
    // // Process a calibration update
    // web_server->on("/calibration/update/", HTTP_POST, [&]() {
    //     processCalibration(request);
    // });


    // File not found handler
    web_server->onNotFound([&]() {
        String pathWithGz = web_server->uri() + ".gz";
        if (web_server->method() == HTTP_OPTIONS) {
            web_server->send(200);
        } else if(FILESYSTEM.exists(web_server->uri()) || FILESYSTEM.exists(pathWithGz)) {
            // WebServer doesn't automatically serve files, so we need to do that here unless we want to
            // manually add every single file to setStaticPages(). 
            handleFileRead(web_server->uri());
        } else {
            Log.verbose(F("Serving 404 for request to %s.\r\n"), web_server->uri().c_str());
            redirect("/404/");
        }
    });

    // DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

    web_server->begin();
    Log.notice(F("HTTP server started. Open: http://%s.local/ to view application.\r\n"), WiFi.getHostname());
}
