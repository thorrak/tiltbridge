#include "resetreasons.h"
#include "http_server.h"

httpServer http_server;
Ticker sendNowTicker;


extern bool send_brewersFriend;
extern bool send_brewfather;
extern bool send_localTarget;
extern bool send_brewStatus;
extern bool send_gSheets;
extern bool send_mqtt;


AsyncWebServer server(WEBPORT);

// This is to simplify the redirects in processCalibration
void redirectToCalibration(AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(301);
    response->addHeader("Location", "/calibration/");
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
}

void processCalibrationError(AsyncWebServerRequest *request) {
    Log.error(F("Error in processCalibration." CR));
    redirectToCalibration(request);
}

// Settings Page Handlers
bool processTiltBridgeSettings(AsyncWebServerRequest *request) {
    int failCount = 0;
    bool hostnamechanged = false;
    // Loop through all parameters
    int params = request->params();
    for (int i = 0; i < params; i++) {
        AsyncWebParameter *p = request->getParam(i);
        if (p->isPost()) {
            // Process any p->name().c_str() / p->value().c_str() pairs
            const char *name = p->name().c_str();
            const char *value = p->value().c_str();
            Log.verbose(F("Processing [%s]:(%s) pair." CR), name, value);

            // Controller settings
            //
            if (strcmp(name, "mdnsID") == 0) {
                // Set hostname
                LCBUrl url;
                if (!url.isValidLabel(value)) {
                    Log.warning(F("Settings update error, [%s]:(%s) not valid." CR), name, value);
                    failCount++;
                } else {
                    if (strcmp(config.mdnsID, value) != 0) {
                        hostnamechanged = true;
                    }
                    Log.notice(F("Settings update, [%s]:(%s) applied." CR), name, value);
                    strlcpy(config.mdnsID, value, 32);
                }
            }
            if (strcmp(name, "tzOffset") == 0) {
                // Set the timezone offset
                const int val = atof(value);
                if ((val <= -12) || (val >= 14)) {
                    Log.warning(F("Settings update error, [%s]:(%s) not valid." CR), name, value);
                    failCount++;
                } else {
                    Log.notice(F("Settings update, [%s]:(%s) applied." CR), name, value);
                    config.TZoffset = val;
                }
            }
            if (strcmp(name, "tempUnit") == 0) {
                // Set temp unit
                if ((strcmp(value, "F") == 1) && (strcmp(value, "F") == 1)) {
                    Log.warning(F("Settings update error, [%s]:(%s) not valid." CR), name, value);
                    failCount++;
                } else {
                    Log.notice(F("Settings update, [%s]:(%s) applied." CR), name, value);
                    strlcpy(config.tempUnit, value, 2);
                }
            }
            if (strcmp(name, "smoothFactor") == 0) {
                // Set the smoothing factor
                const int val = atof(value);
                if ((val < 0) || (val > 99)) {
                    Log.warning(F("Settings update error, [%s]:(%s) not valid." CR), name, value);
                    failCount++;
                } else {
                    Log.notice(F("Settings update, [%s]:(%s) applied." CR), name, value);
                    config.smoothFactor = val;
                }
            }
            if (strcmp(name, "invertTFT") == 0) {
                // Invert TFT orientation
                if (strcmp(value, "true") == 0) {
                    if (!config.invertTFT) {
                        config.invertTFT = true;
                        http_server.lcd_reinit_rqd = true;
                    }
                    Log.notice(F("Settings update, [%s]:(%s) applied." CR), name, value);
                } else if (strcmp(value, "false") == 0) {
                    if (config.invertTFT) {
                        config.invertTFT = false;
                        http_server.lcd_reinit_rqd = true;
                    }
                    Log.notice(F("Settings update, [%s]:(%s) applied." CR), name, value);
                } else {
                    Log.warning(F("Settings update error, [%s]:(%s) not valid." CR), name, value);
                    failCount++;
                }
            }
        }
    }
    if (failCount) {
        Log.error(F("Error: Invalid controller configuration." CR));
        return false;
    } else {
        if (saveConfig()) {
            if (hostnamechanged) {
                // We reset hostname, process
                hostnamechanged = false;
                http_server.name_reset_requested = true;
                Log.notice(F("POSTed new mDNSid, queued network reset." CR));
            }
            return true;
        } else {
            Log.error(F("Error: Unable to save controller configuration data." CR));
            return false;
        }
    }
}

bool processCalibrationSettings(AsyncWebServerRequest *request) {
    int failCount = 0;
    // Loop through all parameters
    int params = request->params();
    for (int i = 0; i < params; i++) {
        AsyncWebParameter *p = request->getParam(i);
        if (p->isPost()) {
            // Process any p->name().c_str() / p->value().c_str() pairs
            const char *name = p->name().c_str();
            const char *value = p->value().c_str();
            Log.verbose(F("Processing [%s]:(%s) pair." CR), name, value);

            // Calibration settings
            //
            if (strcmp(name, "applyCalibration") == 0) {
                // Set apply calibration
                if (strcmp(value, "true") == 0) {
                    config.applyCalibration = true;
                    Log.notice(F("Settings update, [%s]:(%s) applied." CR), name, value);
                } else if (strcmp(value, "false") == 0) {
                    config.applyCalibration = false;
                    Log.notice(F("Settings update, [%s]:(%s) applied." CR), name, value);
                } else {
                    failCount++;
                    Log.warning(F("Settings update error, [%s]:(%s) not valid." CR), name, value);
                }
            }
            if (strcmp(name, "tempCorrect") == 0) {
                // Set apply temperature correction
                if (strcmp(value, "true") == 0) {
                    config.tempCorrect = true;
                    Log.notice(F("Settings update, [%s]:(%s) applied." CR), name, value);
                } else if (strcmp(value, "false") == 0) {
                    config.tempCorrect = false;
                    Log.notice(F("Settings update, [%s]:(%s) applied." CR), name, value);
                } else {
                    failCount++;
                    Log.warning(F("Settings update error, [%s]:(%s) not valid." CR), name, value);
                }
            }
        }
    }
    if (failCount) {
        Log.error(F("Error: Invalid Local Target configuration." CR));
        return false;
    } else {
        if (saveConfig()) {
            return true;
        } else {
            Log.error(F("Error: Unable to save Local Target configuration data." CR));
            return false;
        }
    }
}

bool processLocalTargetSettings(AsyncWebServerRequest *request) {
    int failCount = 0;
    // Loop through all parameters
    int params = request->params();
    for (int i = 0; i < params; i++) {
        AsyncWebParameter *p = request->getParam(i);
        if (p->isPost()) {
            // Process any p->name().c_str() / p->value().c_str() pairs
            const char *name = p->name().c_str();
            const char *value = p->value().c_str();
            Log.verbose(F("Processing [%s]:(%s) pair." CR), name, value);

            // Local target settings
            //
            if (strcmp(name, "localTargetURL") == 0) {
                // Set target URL
                String isURL = value;
                if ((strlen(value) > 3) && (strlen(value) < 255) && isURL.startsWith("http")) {
                    Log.notice(F("Settings update, [%s]:(%s) applied." CR), name, value);
                    strlcpy(config.localTargetURL, value, 256);
                    // Trigger a send to Fermentrack/BPR in 5 seconds using the updated URL
                    sendNowTicker.once(5, [](){send_localTarget = true;});
                } else if (strcmp(value, "") == 0 || strlen(value) == 0) {
                    Log.notice(F("Settings update, [%s]:(%s) cleared." CR), name, value);
                    strlcpy(config.localTargetURL, value, 256);
                } else {
                    failCount++;
                    Log.warning(F("Settings update error, [%s]:(%s) not valid." CR), name, value);
                }
            }
            if (strcmp(name, "localTargetPushEvery") == 0) {
                // Set the push frequency in seconds
                const double val = atof(value);
                if ((val < 15) || (val > 3600)) {
                    failCount++;
                    Log.warning(F("Settings update error, [%s]:(%s) not valid." CR), name, value);
                } else {
                    Log.notice(F("Settings update, [%s]:(%s) applied." CR), name, value);
                    config.localTargetPushEvery = val;
                }
            }
        }
    }
    if (failCount) {
        Log.error(F("Error: Invalid Local Target configuration." CR));
        return false;
    } else {
        if (saveConfig()) {
            return true;
        } else {
            Log.error(F("Error: Unable to save Local Target configuration data." CR));
            return false;
        }
    }
}

bool processGoogleSheetsSettings(AsyncWebServerRequest *request) {
    int failCount = 0;
    // Loop through all parameters
    int params = request->params();
    for (int i = 0; i < params; i++) {
        AsyncWebParameter *p = request->getParam(i);
        if (p->isPost()) {
            // Process any p->name().c_str() / p->value().c_str() pairs
            const char *name = p->name().c_str();
            const char *value = p->value().c_str();
            Log.verbose(F("Processing [%s]:(%s) pair." CR), name, value);

            // Google Sheets settings
            //
            if (strcmp(name, "scriptsURL") == 0) {
                // Set Google Sheets URL
                if (strlen(value) > 3 && strlen(value) < 255 && strncmp(value, "https://script.google.com/", 26) == 0) {
                    strlcpy(config.scriptsURL, value, 256);
                    Log.notice(F("Settings update, [%s]:(%s) applied." CR), name, value);
                    // Trigger a send in 5 seconds using the updated GSheets URL
                    sendNowTicker.once(5, [](){send_gSheets = true;});
                } else if (strcmp(value, "") == 0 || strlen(value) == 0) {
                    strlcpy(config.scriptsURL, value, 256);
                    Log.notice(F("Settings update, [%s]:(%s) cleared." CR), name, value);
                } else {
                    failCount++;
                    Log.warning(F("Settings update error, [%s]:(%s) not valid." CR), name, value);
                }
            }
            if (strcmp(name, "scriptsEmail") == 0) {
                // Set Google Sheets Email
                if (strlen(value) > 7 && strlen(value) < 255) {
                    strlcpy(config.scriptsEmail, value, 256);
                    Log.notice(F("Settings update, [%s]:(%s) applied." CR), name, value);
                } else if (strcmp(value, "") == 0 || strlen(value) == 0) {
                    strlcpy(config.scriptsEmail, value, 256);
                    Log.notice(F("Settings update, [%s]:(%s) cleared." CR), name, value);
                } else {
                    failCount++;
                    Log.warning(F("Settings update error, [%s]:(%s) not valid." CR), name, value);
                }
            }
            if(strstr(name, "sheetName_") != NULL) {
                // Set sheet name
                if (strlen(value) < 25) {
                    // Basically, we're switching on color here and writing to the appropriate config variable
                    if(strstr(name, "_red") != NULL) strlcpy(config.sheetName_red, value, 25);
                    else if(strstr(name, "_green") != NULL) strlcpy(config.sheetName_green, value, 25);
                    else if(strstr(name, "_black") != NULL) strlcpy(config.sheetName_black, value, 25);
                    else if(strstr(name, "_purple") != NULL) strlcpy(config.sheetName_purple, value, 25);
                    else if(strstr(name, "_orange") != NULL) strlcpy(config.sheetName_orange, value, 25);
                    else if(strstr(name, "_yellow") != NULL) strlcpy(config.sheetName_yellow, value, 25);
                    else if(strstr(name, "_blue") != NULL) strlcpy(config.sheetName_blue, value, 25);
                    else if(strstr(name, "_pink") != NULL) strlcpy(config.sheetName_pink, value, 25);
                    else {
                        failCount++;
                        Log.warning(F("Settings update error, invalid color [%s]:(%s) not valid." CR), name, value);
                    }

                    // Technically this will appear after a successful application of a color above. This could be
                    // skipped by doing different logical routing/using bools, but that seems more trouble than its
                    // worth
                    Log.notice(F("Settings updated if color valid [%s]:(%s) applied." CR), name, value);
                } else {
                    failCount++;
                    Log.warning(F("Settings update error, [%s]:(%s) not valid." CR), name, value);
                }
            }

        }
    }
    if (failCount) {
        Log.error(F("Error: Invalid Google Sheets configuration." CR));
        return false;
    } else {
        if (saveConfig()) {
            return true;
        } else {
            Log.error(F("Error: Unable to save Google Sheets configuration data." CR));
            return false;
        }
    }
}

bool processBrewersFriendSettings(AsyncWebServerRequest *request) {
    int failCount = 0;
    // Loop through all parameters
    int params = request->params();
    for (int i = 0; i < params; i++) {
        AsyncWebParameter *p = request->getParam(i);
        if (p->isPost()) {
            // Process any p->name().c_str() / p->value().c_str() pairs
            const char *name = p->name().c_str();
            const char *value = p->value().c_str();
            Log.verbose(F("Processing [%s]:(%s) pair." CR), name, value);

            // Brewer's Friend settings
            //
            if (strcmp(name, "brewersFriendKey") == 0) {
                // Set Brewer's Friend Key
                if (BREWERS_FRIEND_MIN_KEY_LENGTH < strlen(value) && strlen(value) < 255) {
                    strlcpy(config.brewersFriendKey, value, 65);
                    Log.notice(F("Settings update, [%s]:(%s) applied." CR), name, value);
                    // Trigger a send to Brewers Friend in 5 seconds using the updated key
                    sendNowTicker.once(5, [](){send_brewersFriend = true;});
                } else if (strcmp(value, "") == 0 || strlen(value) == 0) {
                    strlcpy(config.brewersFriendKey, value, 65);
                    Log.notice(F("Settings update, [%s]:(%s) cleared." CR), name, value);
                } else {
                    failCount++;
                    Log.warning(F("Settings update error, [%s]:(%s) not valid." CR), name, value);
                }
            }
        }
    }
    if (failCount) {
        Log.error(F("Error: Invalid Brewer's Friend configuration." CR));
        return false;
    } else {
        if (saveConfig()) {
            return true;
        } else {
            Log.error(F("Error: Unable to save Brewer's configuration data." CR));
            return false;
        }
    }
}

bool processBrewfatherSettings(AsyncWebServerRequest *request)
{
    int failCount = 0;
    // Loop through all parameters
    int params = request->params();
    for (int i = 0; i < params; i++) {
        AsyncWebParameter *p = request->getParam(i);
        if (p->isPost()) {
            // Process any p->name().c_str() / p->value().c_str() pairs
            const char *name = p->name().c_str();
            const char *value = p->value().c_str();
            Log.verbose(F("Processing [%s]:(%s) pair." CR), name, value);

            // Brewfather settings
            //
            if (strcmp(name, "brewfatherKey") == 0) {
                // Set Brewfather Key
                if (strlen(value) > BREWERS_FRIEND_MIN_KEY_LENGTH && strlen(value) < 255 ) {
                    strlcpy(config.brewfatherKey, value, 65);
                    Log.notice(F("Settings update, [%s]:(%s) applied." CR), name, value);
                    // Trigger a send to Brewfather in 5 seconds using the updated key
                    sendNowTicker.once(5, [](){send_brewfather = true;});
                } else if (strcmp(value, "") == 0 || strlen(value) == 0) {
                    strlcpy(config.brewfatherKey, value, 65);
                    Log.notice(F("Settings update, [%s]:(%s) cleared." CR), name, value);
                } else {
                    failCount++;
                    Log.warning(F("Settings update error, [%s]:(%s) not valid." CR), name, value);
                }
            }
        }
    }
    if (failCount) {
        Log.error(F("Error: Invalid Brewfather configuration." CR));
        return false;
    } else {
        if (saveConfig()) {
            return true;
        } else {
            Log.error(F("Error: Unable to save Brewfather configuration data." CR));
            return false;
        }
    }
}

bool processBrewstatusSettings(AsyncWebServerRequest *request) {
    int failCount = 0;
    // Loop through all parameters
    int params = request->params();
    for (int i = 0; i < params; i++) {
        AsyncWebParameter *p = request->getParam(i);
        if (p->isPost()) {
            // Process any p->name().c_str() / p->value().c_str() pairs
            const char *name = p->name().c_str();
            const char *value = p->value().c_str();
            Log.verbose(F("Processing [%s]:(%s) pair." CR), name, value);

            // Brewstatus settings
            //
            if (strcmp(name, "brewstatusURL") == 0) {
                // Set Brewstatus Key
                if (strlen(value) > BREWSTATUS_MIN_KEY_LENGTH && strlen(value) < 255) {
                    strlcpy(config.brewstatusURL, value, 256);
                    Log.notice(F("Settings update, [%s]:(%s) applied." CR), name, value);
                    // Trigger a send to Brewstatus in 5 seconds using the updated key
                    sendNowTicker.once(5, [](){send_brewStatus = true;});
                } else if (strcmp(value, "") == 0 || strlen(value) == 0) {
                    strlcpy(config.brewstatusURL, value, 256);
                    Log.notice(F("Settings update, [%s]:(%s) cleared." CR), name, value);
                } else {
                    failCount++;
                    Log.warning(F("Settings update error, [%s]:(%s) not valid." CR), name, value);
                }
            }
            if (strcmp(name, "brewstatusPushEvery") == 0) {
                // Set the push frequency in seconds
                const double val = atof(value);
                if ((val < 30) || (val > 3600)) {
                    failCount++;
                    Log.warning(F("Settings update error, [%s]:(%s) not valid." CR), name, value);
                } else {
                    Log.notice(F("Settings update, [%s]:(%s) applied." CR), name, value);
                    config.brewstatusPushEvery = val;
                }
            }
        }
    }
    if (failCount) {
        Log.error(F("Error: Invalid Brewstatus configuration." CR));
        return false;
    } else {
        if (saveConfig()) {
            return true;
        } else {
            Log.error(F("Error: Unable to save Brewstatus configuration data." CR));
            return false;
        }
    }
}

bool processMqttSettings(AsyncWebServerRequest *request) {
    int failCount = 0;
    // Loop through all parameters
    int params = request->params();
    for (int i = 0; i < params; i++) {
        AsyncWebParameter *p = request->getParam(i);
        if (p->isPost()) {
            // Process any p->name().c_str() / p->value().c_str() pairs
            const char *name = p->name().c_str();
            const char *value = p->value().c_str();
            Log.verbose(F("Processing [%s]:(%s) pair." CR), name, value);

            // MQTT settings
            //
            if (strcmp(name, "mqttBrokerHost") == 0) {
                // Set MQTT address
                LCBUrl url;
                if (strcmp(value, "") == 0 || strlen(value) == 0) {
                    strlcpy(config.mqttBrokerHost, value, 256);
                    http_server.mqtt_init_rqd = true;
                    Log.notice(F("Settings update, [%s]:(%s) cleared." CR), name, value);
                } else if (!url.isValidHostName(value) || (strlen(value) < 3 || strlen(value) > 254)) {
                    failCount++;
                    Log.warning(F("Settings update error, [%s]:(%s) not valid." CR), name, value);
                } else {
                    strlcpy(config.mqttBrokerHost, value, 256);
                    http_server.mqtt_init_rqd = true;
                    Log.notice(F("Settings update, [%s]:(%s) applied." CR), name, value);
                }
            }
            if (strcmp(name, "mqttBrokerPort") == 0) {
                // Set port
                const double val = atof(value);
                if ((val <= 1024) || (val >= 65535)) {
                    failCount++;
                    Log.warning(F("Settings update error, [%s]:(%s) not valid." CR), name, value);
                } else {
                    config.mqttBrokerPort = val;
                    http_server.mqtt_init_rqd = true;
                    Log.notice(F("Settings update, [%s]:(%s) applied." CR), name, value);
                }
            }
            if (strcmp(name, "mqttPushEvery") == 0) {
                // Set frequency in seconds
                const double val = atof(value);
                if ((val < 30) || (val > 3600)) {
                    failCount++;
                    Log.warning(F("Settings update error, [%s]:(%s) not valid." CR), name, value);
                } else {
                    config.mqttPushEvery = val;
                    http_server.mqtt_init_rqd = true;
                    Log.notice(F("Settings update, [%s]:(%s) applied." CR), name, value);
                }
            }
            if (strcmp(name, "mqttUsername") == 0) {
                // Set MQTT User name
                if (strlen(value) > 3 && strlen(value) < 50) {
                    strlcpy(config.mqttUsername, value, 51);
                    http_server.mqtt_init_rqd = true;
                    Log.notice(F("Settings update, [%s]:(%s) applied." CR), name, value);
                } else if (strcmp(value, "") == 0 || strlen(value) == 0) {
                    strlcpy(config.mqttUsername, value, 51);
                    http_server.mqtt_init_rqd = true;
                    Log.notice(F("Settings update, [%s]:(%s) cleared." CR), name, value);
                } else {
                    failCount++;
                    Log.warning(F("Settings update error, [%s]:(%s) not valid." CR), name, value);
                }
            }
            if (strcmp(name, "mqttPassword") == 0) {
                // Set MQTT password
                if (strlen(value) < 64) {
                    strlcpy(config.mqttPassword, value, 65);
                    http_server.mqtt_init_rqd = true;
                    Log.notice(F("Settings update, [%s]:(%s) applied." CR), name, value);
                } else if (strcmp(value, "") == 0 || strlen(value) == 0) {
                    strlcpy(config.mqttPassword, value, 65);
                    http_server.mqtt_init_rqd = true;
                    Log.notice(F("Settings update, [%s]:(%s) cleared." CR), name, value);
                } else {
                    failCount++;
                    Log.warning(F("Settings update error, [%s]:(%s) not valid." CR), name, value);
                }
            }
            if (strcmp(name, "mqttTopic") == 0) {
                // Set MQTT Topic
                if (strlen(value) > 3 && strlen(value) < 30) {
                    strlcpy(config.mqttTopic, value, 31);
                    http_server.mqtt_init_rqd = true;
                    Log.notice(F("Settings update, [%s]:(%s) applied." CR), name, value);
                } else if (strcmp(value, "") == 0 || strlen(value) == 0) {
                    strlcpy(config.mqttTopic, "tiltbridge", 31);
                    http_server.mqtt_init_rqd = true;
                    Log.notice(F("Settings update, [%s]:(%s) cleared." CR), name, value);
                } else {
                    failCount++;
                    Log.warning(F("Settings update error, [%s]:(%s) not valid." CR), name, value);
                }
            }
        }
    }
    if (failCount) {
        Log.error(F("Error: Invalid MQTT configuration." CR));
        return false;
    } else {
        if (saveConfig()) {
            // Trigger a send via MQTT in 5 seconds using the updated data
            sendNowTicker.once(5, [](){send_mqtt = true;});
            return true;
        } else {
            Log.error(F("Error: Unable to save MQTT configuration data." CR));
            return false;
        }
    }
}

constexpr unsigned int str2int(const char *str, int h = 0) {
    return !str[h] ? 5381 : (str2int(str, h + 1) * 33) ^ str[h];
}

// we don't need to do much input checking on the calibration data as we are
// looking at numbers generated by the javascript and not a human
void processCalibration(AsyncWebServerRequest *request) {
    int tilt_color_no = 0;
    int degree;
    double x0, x1, x2, x3;

    if (request->hasArg("clearTiltColor")) {
        tilt_color_no = str2int(request->arg("clearTiltColor").c_str());
        degree = 1;
        x1 = 1.0;
        x0 = x2 = x3 = 0.0;
    }

    if (request->hasArg("updateTiltColor")) {
        tilt_color_no = str2int(request->arg("updateTiltColor").c_str());
        if (request->hasArg("linear")) {
            degree = 1;
            x0 = strtod(request->arg("linearFitx0").c_str(), nullptr);
            x1 = strtod(request->arg("linearFitx1").c_str(), nullptr);
            x2 = x3 = 0.0;
        } else if (request->hasArg("quadratic")) {
            degree = 2;
            x0 = strtod(request->arg("quadraticFitx0").c_str(), nullptr);
            x1 = strtod(request->arg("quadraticFitx1").c_str(), nullptr);
            x2 = strtod(request->arg("quadraticFitx2").c_str(), nullptr);
            x3 = 0.0;
        } else if (request->hasArg("cubic")) {
            degree = 2;
            x0 = strtod(request->arg("cubicFitx0").c_str(), nullptr);
            x1 = strtod(request->arg("cubicFitx1").c_str(), nullptr);
            x2 = strtod(request->arg("cubicFitx2").c_str(), nullptr);
            x3 = strtod(request->arg("cubicFitx3").c_str(), nullptr);
        } else {
            processCalibrationError(request);
        }
    }

    if(0 <= tilt_color_no && tilt_color_no < TILT_COLORS) {
        config.tilt_calibration[tilt_color_no].degree = degree;
        config.tilt_calibration[tilt_color_no].x0 = x0;
        config.tilt_calibration[tilt_color_no].x1 = x1;
        config.tilt_calibration[tilt_color_no].x2 = x2;
        config.tilt_calibration[tilt_color_no].x3 = x3;
    } else {
        processCalibrationError(request);
    }

    redirectToCalibration(request);
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

void http_json(AsyncWebServerRequest *request) {
    Log.verbose(F("Serving Tilt JSON." CR));
    char tilt_data[TILT_ALL_DATA_SIZE];
    tilt_scanner.tilt_to_json_string(tilt_data, false);
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", tilt_data);
    request->send(response);
}

void settings_json(AsyncWebServerRequest *request) {
    Log.verbose(F("Serving settings JSON." CR));
    DynamicJsonDocument doc(capacitySerial);
    JsonObject root = doc.to<JsonObject>();
    config.save(root);

    String config_js;
    serializeJson(doc, config_js);
    
    request->send(200, "application/json", config_js);
}

// About.htm page Handlers
//

void this_version(AsyncWebServerRequest *request) {
    Log.verbose(F("Serving version." CR));
    StaticJsonDocument<96> doc;

    doc["version"] = version();
    doc["branch"] = branch();
    doc["build"] = build();

    char output[96];
    serializeJson(doc, output);

    request->send(200, "application/json", output);
}

void uptime(AsyncWebServerRequest *request) {
    Log.verbose(F("Serving uptime." CR));
    StaticJsonDocument<96> doc;

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

    char output[96];
    serializeJson(doc, output);

    request->send(200, "application/json", output);
}

void heap(AsyncWebServerRequest *request) {
    Log.verbose(F("Serving heap information." CR));
    StaticJsonDocument<48> doc;

    const uint32_t free = ESP.getFreeHeap();
    const uint32_t max = ESP.getMaxAllocHeap();
    const uint8_t frag = 100 - (max * 100) / free;

    doc["free"] = free;
    doc["max"] = max;
    doc["frag"] = frag;

    char output[48];
    serializeJson(doc, output);

    request->send(200, "application/json", output);
}

void reset_reason(AsyncWebServerRequest *request) {
    Log.verbose(F("Serving reset reason." CR));
    StaticJsonDocument<128> doc;

    const int reset = (int)esp_reset_reason();

    doc["reason"] = resetReason[reset];
    doc["description"] = resetDescription[reset];

    char output[128];
    serializeJson(doc, output);

    request->send(200, "application/json", output);
}

void setStaticPages() {
    // Static page handlers
    server.serveStatic("/", FILESYSTEM, "/").setDefaultFile("index.htm").setCacheControl("max-age=600");
    server.serveStatic("/index/", FILESYSTEM, "/").setDefaultFile("index.htm").setCacheControl("max-age=600");
    server.serveStatic("/settings/", FILESYSTEM, "/").setDefaultFile("settings.htm").setCacheControl("max-age=600");
    server.serveStatic("/calibration/", FILESYSTEM, "/").setDefaultFile("calibration.htm").setCacheControl("max-age=600");
    server.serveStatic("/help/", FILESYSTEM, "/").setDefaultFile("help.htm").setCacheControl("max-age=600");
    server.serveStatic("/about/", FILESYSTEM, "/").setDefaultFile("about.htm").setCacheControl("max-age=600");
    server.serveStatic("/controllerrestart/", FILESYSTEM, "/").setDefaultFile("controllerrestart.htm").setCacheControl("max-age=600");
    server.serveStatic("/wifireset/", FILESYSTEM, "/").setDefaultFile("wifireset.htm").setCacheControl("max-age=600");
    server.serveStatic("/factoryreset/", FILESYSTEM, "/").setDefaultFile("factoryreset.htm").setCacheControl("max-age=600");
    server.serveStatic("/404/", FILESYSTEM, "/").setDefaultFile("404.htm").setCacheControl("max-age=600");
}

void setPostPages() {
    // Settings Page Handlers
    server.on("/settings/controller/", HTTP_POST, [](AsyncWebServerRequest *request) {
        Log.verbose(F("Processing post to /settings/controller/." CR));
        if (processTiltBridgeSettings(request)) {
            request->send(200, F("text/plain"), F("Ok"));
        } else {
            request->send(500, F("text/plain"), F("Unable to process data"));
        }
    });
    server.on("/settings/calibration/", HTTP_POST, [](AsyncWebServerRequest *request) {
        Log.verbose(F("Processing post to /settings/calibration/." CR));
        if (processCalibrationSettings(request)) {
            request->send(200, F("text/plain"), F("Ok"));
        } else {
            request->send(500, F("text/plain"), F("Unable to process data"));
        }
    });
    server.on("/settings/localtarget/", HTTP_POST, [](AsyncWebServerRequest *request) {
        Log.verbose(F("Processing post to /settings/localtarget/." CR));
        if (processLocalTargetSettings(request)) {
            request->send(200, F("text/plain"), F("Ok"));
        } else {
            request->send(500, F("text/plain"), F("Unable to process data"));
        }
    });
    server.on("/settings/googlesheets/", HTTP_POST, [](AsyncWebServerRequest *request) {
        Log.verbose(F("Processing post to /settings/googlesheets/." CR));
        if (processGoogleSheetsSettings(request)) {
            request->send(200, F("text/plain"), F("Ok"));
        } else {
            request->send(500, F("text/plain"), F("Unable to process data"));
        }
    });
    server.on("/settings/brewersfriend/", HTTP_POST, [](AsyncWebServerRequest *request) {
        Log.verbose(F("Processing post to /settings/brewersfriend/." CR));
        if (processBrewersFriendSettings(request)) {
            request->send(200, F("text/plain"), F("Ok"));
        } else {
            request->send(500, F("text/plain"), F("Unable to process data"));
        }
    });
    server.on("/settings/brewfather/", HTTP_POST, [](AsyncWebServerRequest *request) {
        Log.verbose(F("Processing post to /settings/brewfather/." CR));
        if (processBrewfatherSettings(request)) {
            request->send(200, F("text/plain"), F("Ok"));
        } else {
            request->send(500, F("text/plain"), F("Unable to process data"));
        }
    });
    server.on("/settings/brewstatus/", HTTP_POST, [](AsyncWebServerRequest *request) {
        Log.verbose(F("Processing post to /settings/brewstatus/." CR));
        if (processBrewstatusSettings(request)) {
            request->send(200, F("text/plain"), F("Ok"));
        } else {
            request->send(500, F("text/plain"), F("Unable to process data"));
        }
    });
    server.on("/settings/mqtt/", HTTP_POST, [](AsyncWebServerRequest *request) {
        Log.verbose(F("Processing post to /settings/mqtt/." CR));
        if (processMqttSettings(request)) {
            request->send(200, F("text/plain"), F("Ok"));
        } else {
            request->send(500, F("text/plain"), F("Unable to process data"));
        }
    });
}

void setJsonPages() {
    // Tilt JSON
    server.on("/json/", HTTP_GET, [](AsyncWebServerRequest *request) {
        http_json(request);
    });

    // Settings JSON
    server.on("/settings/json/", HTTP_GET, [](AsyncWebServerRequest *request) {
        settings_json(request);
    });

    // About Page JSON
    server.on("/thisVersion/", HTTP_GET, [](AsyncWebServerRequest *request) {
        this_version(request);
    });
    server.on("/uptime/", HTTP_GET, [](AsyncWebServerRequest *request) {
        uptime(request);
    });
    server.on("/heap/", HTTP_GET, [](AsyncWebServerRequest *request) {
        heap(request);
    });
    server.on("/resetreason/", HTTP_GET, [](AsyncWebServerRequest *request) {
        reset_reason(request);
    });
}

void setActionPages() {
#ifndef DISABLE_OTA_UPDATES
    server.on("/ota/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, F("text/plain"), F("Ok."));
        trigger_OTA(request);
    });
#endif

    server.on("/resetwifi/", HTTP_GET, [](AsyncWebServerRequest *request) {
        Log.verbose(F("Processing /resetwifi/." CR));
        request->send(200, F("text/plain"), F("Ok."));
        http_server.wifi_reset_requested = true;
    });

    server.on("/resetapp/", HTTP_GET, [](AsyncWebServerRequest *request) {
        Log.verbose(F("Processing /resetapp/." CR));
        request->send(200, F("text/plain"), F("Ok."));
        http_server.factoryreset_requested = true;
    });

    server.on("/oktoreset/", HTTP_GET, [](AsyncWebServerRequest *request) {
        Log.verbose(F("Processing /oktoreset/." CR));
        request->send(200, F("text/plain"), F("Ok."));
        http_server.restart_requested = true;
    });

    server.on("/ping/", HTTP_ANY, [](AsyncWebServerRequest *request) {
        Log.verbose(F("Processing /ping/." CR));
        request->send(200, F("text/plain"), F("Ok."));
    });
}

void httpServer::init() {
    setStaticPages();
    setPostPages();
    setJsonPages();
    setActionPages();

    // Process a calibration update
    server.on("/calibration/update/", HTTP_POST, [](AsyncWebServerRequest *request) {
        processCalibration(request);
    });

#ifdef FSEDIT
#warning "Filesystem editor is enabled! Disable before release."
    // Setup Filesystem editor
    server.addHandler(new SPIFFSEditor(FILESYSTEM, "admin", "p@ssword"));

    server.on("/edit/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("/edit");
    });
#endif

    // File not found handler
    server.onNotFound([](AsyncWebServerRequest *request) {
        if (request->method() == HTTP_OPTIONS) {
            request->send(200);
        } else {
            Log.verbose(F("Serving 404 for request to %s." CR), request->url().c_str());
            request->redirect("/404/");
        }
    });

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

    server.begin();
    Log.notice(F("HTTP server started. Open: http://%s.local/ to view application." CR), WiFi.getHostname());
}
