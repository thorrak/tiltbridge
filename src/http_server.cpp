//
// Created by John Beeler on 2/17/19.
// Modified by Tim Pletcher 31-Oct-2020.
//

#include "resetreasons.h"
#include "http_server.h"

httpServer http_server;

AsyncWebServer server(80);

static char all_valid[2] = "1";

//void trigger_restart();
void trigger_restart(AsyncWebServerRequest *request);

void isInteger(const char *s, bool &is_int, int32_t &int_value)
{
    if ((strlen(s) <= 0) || (!isdigit(s[0])))
    {
        is_int = false;
    }
    char *p;
    int_value = strtol(s, &p, 10);
    is_int = (*p == 0);
}

bool isvalidAddress(const char *s)
{
    //Rudimentary check that the address is of the form aaa.bbb.ccc or
    //aaa.bbb (if DNS) or 111.222.333.444 (if IP). Will not currently catch if integer > 254
    // is input when using IP address
    if (strlen(s) > 253)
    {
        return false;
    }
    int seg_ct = 0;
    char ts[strlen(s) + 1];
    strcpy(ts, s);
    char *item = strtok(ts, ".");
    while (item != NULL)
    {
        ++seg_ct;
        item = strtok(NULL, ".");
    }
    if ((s[0] == '-') || (s[0] == '.') || (s[strlen(s) - 1] == '-') || (s[strlen(s) - 1] == '.'))
        return false;
    for (int i = 0; i < strlen(s); i++)
    {
        if (seg_ct == 2 || seg_ct == 3)
        { //must be a DNS name if true
            if (!isalnum(s[i]) && s[i] != '.' && s[i] != '-')
                return false;
        }
        else if (seg_ct == 4)
        { //must be an IP if true
            if (!isdigit(s[i]) && s[i] != '.')
                return false;
        }
        else
        {
            return false;
        }
    }
    return true;
}

bool isValidmdnsName(const char *mdns_name)
{
    if (strlen(mdns_name) > 31 || strlen(mdns_name) < 8 || mdns_name[0] == '-' || mdns_name[strlen(mdns_name) - 1] == '-')
        return false;
    for (int i = 0; i < strlen(mdns_name); i++)
    {
        if (!isalnum(mdns_name[i]) && mdns_name[i] != '-')
            return false;
    }
    return true;
}

// This is to simplify the redirects in processConfig
void redirectToConfig(AsyncWebServerRequest *request)
{
    AsyncWebServerResponse *response = request->beginResponse(301);
    response->addHeader("Location", "/settings/");
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
}

void processConfigError(AsyncWebServerRequest *request)
{
    Log.error(F("Error in processConfig." CR));
    all_valid[0] = '0';
    redirectToConfig(request);
}

// This is to simplify the redirects in processCalibration
void redirectToCalibration(AsyncWebServerRequest *request)
{
    AsyncWebServerResponse *response = request->beginResponse(301);
    response->addHeader("Location", "/calibration/");
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
}

void processCalibrationError(AsyncWebServerRequest *request)
{
    Log.error(F("Error in processCalibration." CR));
    redirectToCalibration(request);
}

// TODO - This should be refactored to accept pointers to the string we were passed and the string to wet
void processSheetName(const char *varName, bool &is_empty, bool &is_valid, AsyncWebServerRequest *request)
{
    is_valid = false;
    is_empty = false;
    if (request->hasArg(varName))
    {
        if (request->arg(varName).length() > 64)
        {
            is_empty = false;
            is_valid = false;
        }
        else if (request->arg(varName).length() < 1)
        {
            is_empty = true;
            is_valid = true;
        }
        else
        {
            is_valid = true;
            is_empty = false;
        }
    }
    // True or false is error state - not if it was processed
}

void processConfig(AsyncWebServerRequest *request)
{
    bool restart_tiltbridge = false;
    bool all_settings_valid = true;
    bool reinit_tft = false;
    bool mqtt_broker_update = false;

    // Generic TiltBridge Settings
    if (request->hasArg("mdnsID"))
    {
        if (strncmp(config.mdnsID, request->arg("mdnsID").c_str(), 32) != 0)
        {
            if (isValidmdnsName(request->arg("mdnsID").c_str()))
            {
                strlcpy(config.mdnsID, request->arg("mdnsID").c_str(), 32);
                // When we update the mDNS ID, a lot of things have to get reset. Rather than doing the hard work of actually
                // resetting those settings & broadcasting the new ID, let's just restart the controller.
                restart_tiltbridge = true;
            }
            else
            {
                all_settings_valid = false;
            }
        }
        if (request->arg("TZoffset").length() > 0 && request->arg("TZoffset").length() <= 3)
        {
            int tzo;
            bool is_int;
            isInteger(request->arg("TZoffset").c_str(), is_int, tzo);
            if (tzo >= -12 && tzo <= 14)
            {
                config.TZoffset = tzo;
            }
            else
            {
                Log.error(F("Error: brewstatusTZoffset is not between -12 and 14." CR));
                all_settings_valid = false;
            }
        }

        int sf;
        bool is_int;
        isInteger(request->arg("smoothFactor").c_str(), is_int, sf);
        if (sf != config.smoothFactor)
        {
            if (is_int && sf >= 0 && sf <= 99)
            {
                config.smoothFactor = sf;
            }
            else
            {
                all_settings_valid = false;
            }
        }

        strlcpy(config.tempUnit, request->arg("tempUnit").c_str(), 2);

        if (request->arg("invertTFT") == "on" && !config.invertTFT)
        {
            config.invertTFT = true;
            reinit_tft = true;
        }
        else if (request->arg("invertTFT") == "off" && config.invertTFT)
        {
            config.invertTFT = false;
            reinit_tft = true;
        }
    }

    if (request->hasArg("applyCalibration"))
    {
        if (request->arg("applyCalibration") == "on" && !config.applyCalibration)
        {
            config.applyCalibration = true;
        }
        else if (request->arg("applyCalibration") == "off" && config.applyCalibration)
        {
            config.applyCalibration = false;
        }

        if (request->arg("tempCorrect") == "on" && !config.tempCorrect)
        {
            config.tempCorrect = true;
        }
        else if (request->arg("tempCorrect") == "off" && config.tempCorrect)
        {
            config.tempCorrect = false;
        }
    }

    // LocalTarget Settings
    if (request->hasArg("localTargetURL"))
    {
        if (request->arg("localTargetURL").length() <= 255)
        {
            if (request->arg("localTargetURL").length() < 12)
            {
                strlcpy(config.localTargetURL, "", 2);
            }
            else
            {
                strlcpy(config.localTargetURL, request->arg("localTargetURL").c_str(), 256);
            }
        }
        else
        {
            all_settings_valid = false;
        }

        int push_every;
        bool is_int;
        isInteger(request->arg("localTargetPushEvery").c_str(), is_int, push_every);
        if (is_int && push_every <= 3600 && push_every >= 15)
        {
            config.localTargetPushEvery = push_every;
        }
        else
        {
            all_settings_valid = false;
        }
    }

    // Brewstatus Settings
    if (request->hasArg("brewstatusURL"))
    {
        if (request->arg("brewstatusURL").length() <= 255)
        {
            if (request->arg("brewstatusURL").length() < 12)
            {
                strlcpy(config.brewstatusURL, "", 2);
            }
            else
            {
                strlcpy(config.brewstatusURL, request->arg("brewstatusURL").c_str(), 256);
            }
        }
        else
        {
            all_settings_valid = false;
        }

        int push_every;
        bool is_int;
        isInteger(request->arg("brewstatusPushEvery").c_str(), is_int, push_every);
        if (is_int && push_every <= 3600 && push_every >= 30)
        {
            config.brewstatusPushEvery = push_every;
        }
        else
        {
            all_settings_valid = false;
        }
    }

    // Google Sheets Settings
    if (request->hasArg("scriptsURL"))
    {
        if (request->arg("scriptsURL").length() <= 255)
        {
            if (request->arg("scriptsURL").length() > 12 &&
                (strncmp(request->arg("scriptsURL").c_str(), "https://script.google.com/", 26) == 0))
            {
                strlcpy(config.scriptsURL, request->arg("scriptsURL").c_str(), 256);
            }
            else
            {
                strlcpy(config.scriptsURL, "", 2);
            }
        }
        else
        {
            all_settings_valid = false;
        }

        if (request->arg("scriptsEmail").length() <= 255)
        {
            if (request->arg("scriptsEmail").length() < 7)
            {
                strlcpy(config.scriptsEmail, "", 2);
            }
            else
            {
                strlcpy(config.scriptsEmail, request->arg("scriptsEmail").c_str(), 256);
            }
        }
        else
        {
            all_settings_valid = false;
        }

        // Individual Google Sheets Beer Log Names
        bool is_empty;
        bool is_valid;
        processSheetName("sheetName_red", is_empty, is_valid, request);
        if (!is_valid)
        {
            all_settings_valid = false;
        }
        else if (is_empty)
        {
            strlcpy(config.sheetName_red, "", 2);
        }
        else
        {
            strlcpy(config.sheetName_red, request->arg("sheetName_red").c_str(), 25);
        }
        processSheetName("sheetName_green", is_empty, is_valid, request);
        if (!is_valid)
        {
            all_settings_valid = false;
        }
        else if (is_empty)
        {
            strlcpy(config.sheetName_green, "", 2);
        }
        else
        {
            strlcpy(config.sheetName_green, request->arg("sheetName_green").c_str(), 25);
        }
        processSheetName("sheetName_black", is_empty, is_valid, request);
        if (!is_valid)
        {
            all_settings_valid = false;
        }
        else if (is_empty)
        {
            strlcpy(config.sheetName_black, "", 2);
        }
        else
        {
            strlcpy(config.sheetName_black, request->arg("sheetName_black").c_str(), 25);
        }
        processSheetName("sheetName_purple", is_empty, is_valid, request);
        if (!is_valid)
        {
            all_settings_valid = false;
        }
        else if (is_empty)
        {
            strlcpy(config.sheetName_purple, "", 2);
        }
        else
        {
            strlcpy(config.sheetName_purple, request->arg("sheetName_purple").c_str(), 25);
        }
        processSheetName("sheetName_orange", is_empty, is_valid, request);
        if (!is_valid)
        {
            all_settings_valid = false;
        }
        else if (is_empty)
        {
            strlcpy(config.sheetName_orange, "", 2);
        }
        else
        {
            strlcpy(config.sheetName_orange, request->arg("sheetName_orange").c_str(), 25);
        }
        processSheetName("sheetName_blue", is_empty, is_valid, request);
        if (!is_valid)
        {
            all_settings_valid = false;
        }
        else if (is_empty)
        {
            strlcpy(config.sheetName_blue, "", 2);
        }
        else
        {
            strlcpy(config.sheetName_blue, request->arg("sheetName_blue").c_str(), 25);
        }
        processSheetName("sheetName_yellow", is_empty, is_valid, request);
        if (!is_valid)
        {
            all_settings_valid = false;
        }
        else if (is_empty)
        {
            strlcpy(config.sheetName_yellow, "", 2);
        }
        else
        {
            strlcpy(config.sheetName_yellow, request->arg("sheetName_yellow").c_str(), 25);
        }
        processSheetName("sheetName_pink", is_empty, is_valid, request);
        if (!is_valid)
        {
            all_settings_valid = false;
        }
        else if (is_empty)
        {
            strlcpy(config.sheetName_pink, "", 2);
        }
        else
        {
            strlcpy(config.sheetName_pink, request->arg("sheetName_pink").c_str(), 25);
        }
    }
    // Brewers Friend Setting
    if (request->hasArg("brewersFriendKey"))
    {
        if (request->arg("brewersFriendKey").length() <= 255)
        {
            if (request->arg("brewersFriendKey").length() < BREWERS_FRIEND_MIN_KEY_LENGTH)
            {
                strlcpy(config.brewersFriendKey, "", 2);
            }
            else
            {
                strlcpy(config.brewersFriendKey, request->arg("brewersFriendKey").c_str(), 25);
            }
        }
        else
        {
            all_settings_valid = false;
        }
    }

    // Brewfather
    if (request->hasArg("brewfatherKey"))
    {
        if (request->arg("brewfatherKey").length() <= 255)
        {
            if (request->arg("brewfatherKey").length() < BREWERS_FRIEND_MIN_KEY_LENGTH)
            {
                strlcpy(config.brewfatherKey, "", 2);
            }
            else
            {
                strlcpy(config.brewfatherKey, request->arg("brewfatherKey").c_str(), 25);
            }
        }
        else
        {
            all_settings_valid = false;
        }
    }

    // MQTT
    if (request->hasArg("mqttBrokerIP"))
    {
        if (isvalidAddress(request->arg("mqttBrokerIP").c_str()))
        {
            strlcpy(config.mqttBrokerIP, request->arg("mqttBrokerIP").c_str(), 254);
            mqtt_broker_update = true;
        }
        else if (request->arg("mqttBrokerIP").length() < 2)
        {
            strlcpy(config.mqttBrokerIP, "", 2);
        }
        else
        {
            all_settings_valid = false;
        }

        int port_number;
        bool is_int;
        isInteger(request->arg("mqttBrokerPort").c_str(), is_int, port_number);
        if (is_int && port_number < 65535 && port_number > 1024)
        {
            if (config.mqttBrokerPort != port_number)
            {
                config.mqttBrokerPort = port_number;
                mqtt_broker_update = true;
            }
        }
        else
        {
            all_settings_valid = false;
        }

        int push_every;
        isInteger(request->arg("mqttPushEvery").c_str(), is_int, push_every);
        if (is_int && push_every <= 3600 && push_every >= 15)
        {
            config.mqttPushEvery = push_every;
        }
        else
        {
            all_settings_valid = false;
        }

        if (request->arg("mqttUsername").length() <= 50)
        {
            if (request->arg("mqttUsername").length() <= 0)
            {
                strlcpy(config.mqttUsername, "", 2);
            }
            else
            {
                strlcpy(config.mqttUsername, request->arg("mqttUsername").c_str(), 51);
            }
            mqtt_broker_update = true;
        }
        else
        {
            all_settings_valid = false;
        }

        if (request->arg("mqttPassword").length() <= 128)
        {
            if (request->arg("mqttPassword").length() <= 0)
            {
                strlcpy(config.mqttPassword, "", 2);
            }
            else
            {
                strlcpy(config.mqttPassword, request->arg("mqttPassword").c_str(), 65);
            }
            mqtt_broker_update = true;
        }
        else
        {
            all_settings_valid = false;
        }

        if (request->arg("mqttTopic").length() <= 30)
        {
            if (request->arg("mqttTopic").length() <= 2)
            {
                strlcpy(config.mqttTopic, "tiltbridge", 11);
            }
            else
            {
                strlcpy(config.mqttTopic, request->arg("mqttTopic").c_str(), 31);
            }
        }
        else
        {
            all_settings_valid = false;
        }
    }

    // If we made it this far, one or more settings were updated. Save.
    if (all_settings_valid)
    {
        http_server.config_updated = true;
    }
    else
    {
        return processConfigError(request);
    }

    if (mqtt_broker_update)
    {
        http_server.mqtt_init_rqd = true;
    }

    if (reinit_tft)
    {
        http_server.lcd_init_rqd = true;
    }

    if (restart_tiltbridge)
    {
        trigger_restart(request);
    }
    else
    {
        redirectToConfig(request);
    }
}

constexpr unsigned int str2int(const char *str, int h = 0)
{
    return !str[h] ? 5381 : (str2int(str, h + 1) * 33) ^ str[h];
}

// we don't need to do much input checking on the calibration data as we are
// looking at numbers generated by the javascript and not a human
void processCalibration(AsyncWebServerRequest *request)
{
    int tilt_name = 0;
    int degree;
    double x0, x1, x2, x3;

    if (request->hasArg("clearTiltColor"))
    {
        tilt_name = str2int(request->arg("clearTiltColor").c_str());
        degree = 1;
        x1 = 1.0;
        x0 = x2 = x3 = 0.0;
    }

    if (request->hasArg("updateTiltColor"))
    {
        tilt_name = str2int(request->arg("updateTiltColor").c_str());
        if (request->hasArg("linear"))
        {
            degree = 1;
            x0 = strtod(request->arg("linearFitx0").c_str(), nullptr);
            x1 = strtod(request->arg("linearFitx1").c_str(), nullptr);
            x2 = x3 = 0.0;
        }
        else if (request->hasArg("quadratic"))
        {
            degree = 2;
            x0 = strtod(request->arg("quadraticFitx0").c_str(), nullptr);
            x1 = strtod(request->arg("quadraticFitx1").c_str(), nullptr);
            x2 = strtod(request->arg("quadraticFitx2").c_str(), nullptr);
            x3 = 0.0;
        }
        else if (request->hasArg("cubic"))
        {
            degree = 2;
            x0 = strtod(request->arg("cubicFitx0").c_str(), nullptr);
            x1 = strtod(request->arg("cubicFitx1").c_str(), nullptr);
            x2 = strtod(request->arg("cubicFitx2").c_str(), nullptr);
            x3 = strtod(request->arg("cubicFitx3").c_str(), nullptr);
        }
        else
        {
            processCalibrationError(request);
        }
    }

    switch (tilt_name)
    {
    case str2int("red"):
        config.cal_red_degree = degree;
        config.cal_red_x0 = x0;
        config.cal_red_x1 = x1;
        config.cal_red_x2 = x2;
        config.cal_red_x3 = x3;
        break;
    case str2int("green"):
        config.cal_green_degree = degree;
        config.cal_green_x0 = x0;
        config.cal_green_x1 = x1;
        config.cal_green_x2 = x2;
        config.cal_green_x3 = x3;
        break;
    case str2int("black"):
        config.cal_black_degree = degree;
        config.cal_black_x0 = x0;
        config.cal_black_x1 = x1;
        config.cal_black_x2 = x2;
        config.cal_black_x3 = x3;
        break;
    case str2int("purple"):
        config.cal_purple_degree = degree;
        config.cal_purple_x0 = x0;
        config.cal_purple_x1 = x1;
        config.cal_purple_x2 = x2;
        config.cal_purple_x3 = x3;
        break;
    case str2int("orange"):
        config.cal_orange_degree = degree;
        config.cal_orange_x0 = x0;
        config.cal_orange_x1 = x1;
        config.cal_orange_x2 = x2;
        config.cal_orange_x3 = x3;
        break;
    case str2int("blue"):
        config.cal_blue_degree = degree;
        config.cal_blue_x0 = x0;
        config.cal_blue_x1 = x1;
        config.cal_blue_x2 = x2;
        config.cal_blue_x3 = x3;
        break;
    case str2int("yellow"):
        config.cal_yellow_degree = degree;
        config.cal_yellow_x0 = x0;
        config.cal_yellow_x1 = x1;
        config.cal_yellow_x2 = x2;
        config.cal_yellow_x3 = x3;
        break;
    case str2int("pink"):
        config.cal_pink_degree = degree;
        config.cal_pink_x0 = x0;
        config.cal_pink_x1 = x1;
        config.cal_pink_x2 = x2;
        config.cal_pink_x3 = x3;
        break;
    default:
        processCalibrationError(request);
    }

    redirectToCalibration(request);
}

//-----------------------------------------------------------------------------------------

#ifndef DISABLE_OTA_UPDATES
void trigger_OTA(AsyncWebServerRequest *request)
{
    server.serveStatic("/updating.htm", FILESYSTEM, "/").setDefaultFile("updating.htm");
    config.update_spiffs = true;
    lcd.display_ota_update_screen();         // Trigger this here while everything else is waiting.
    delay(1000);                             // Wait 1 second to let everything send
    tilt_scanner.wait_until_scan_complete(); // Wait for scans to complete (we don't want any tasks running in the background)
    execOTA();                               // Trigger the OTA update
}
#endif

void trigger_wifi_reset(AsyncWebServerRequest *request)
{
    Log.verbose(F("Resetting WiFi." CR));
    request->send(FILESYSTEM, "/restarting.htm", "text/html");
    tilt_scanner.wait_until_scan_complete();    // Wait for scans to complete (we don't want any tasks running in the background)
    // TODO - Come back and refactor this lightly to use similar logic to restart_requested
    disconnect_from_wifi_and_restart(); // Reset the wifi settings
}

void trigger_restart(AsyncWebServerRequest *request)
{
    Log.verbose(F("Resetting controller." CR));
    request->send(FILESYSTEM, "/restarting.htm", "text/html");
    http_server.restart_requested = true;
}

void http_json(AsyncWebServerRequest *request)
{
     // TODO: JSON Go rework this
    char tilt_data[1600];
    tilt_scanner.tilt_to_json_string(tilt_data, false);
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", tilt_data);
    request->send(response);
}

// settings_json is intended to be used to build the "Change Settings" page
void settings_json(AsyncWebServerRequest *request)
{
    DynamicJsonDocument doc(3072);
    JsonObject root = doc.to<JsonObject>();
    config.save(root);

    String config_js;
    serializeJson(doc, config_js);
    
    request->send(200, "application/json", config_js);
}

// About.htm page Handlers
//

void this_version(AsyncWebServerRequest *request)
{
    Log.verbose(F("Serving version." CR));
    DynamicJsonDocument doc(96);

    doc["version"] = version();
    doc["branch"] = branch();
    doc["build"] = build();

    String output;
    serializeJson(doc, output);

    request->send(200, "application/json", output);
}

void uptime(AsyncWebServerRequest *request)
{
    Log.verbose(F("Serving uptime." CR));
    DynamicJsonDocument doc(96);

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

    String output;
    serializeJson(doc, output);

    request->send(200, "application/json", output);
}

void heap(AsyncWebServerRequest *request)
{
    Log.verbose(F("Serving heap information." CR));
    DynamicJsonDocument doc(48);

    const uint32_t free = ESP.getFreeHeap();
    const uint32_t max = ESP.getMaxAllocHeap();
    const uint8_t frag = 100 - (max * 100) / free;

    doc["free"] = free;
    doc["max"] = max;
    doc["frag"] = frag;

    String output;
    serializeJson(doc, output);

    request->send(200, "application/json", output);
}

void reset_reason(AsyncWebServerRequest *request)
{
    Log.verbose(F("Serving reset reason." CR));
    DynamicJsonDocument doc(128);

    const int reset = (int)esp_reset_reason();

    doc["reason"] = resetReason[reset];
    doc["description"] = resetDescription[reset];

    String output;
    serializeJson(doc, output);

    request->send(200, "application/json", output);
}

void httpServer::init()
{
    server.serveStatic("/", FILESYSTEM, "/").setDefaultFile("index.htm").setCacheControl("max-age=600");
    server.serveStatic("/index/", FILESYSTEM, "/").setDefaultFile("index.htm").setCacheControl("max-age=600");
    server.serveStatic("/settings/", FILESYSTEM, "/").setDefaultFile("settings.htm").setCacheControl("max-age=600");
    server.serveStatic("/calibration/", FILESYSTEM, "/").setDefaultFile("calibration.htm").setCacheControl("max-age=600");
    server.serveStatic("/help/", FILESYSTEM, "/").setDefaultFile("help.htm").setCacheControl("max-age=600");
    server.serveStatic("/about/", FILESYSTEM, "/").setDefaultFile("about.htm").setCacheControl("max-age=600");
    server.serveStatic("/404/", FILESYSTEM, "/").setDefaultFile("404.htm").setCacheControl("max-age=600");

    server.on("/settings/update/", HTTP_POST, [](AsyncWebServerRequest *request) {
        processConfig(request);
    });
    server.on("/calibration/update/", HTTP_POST, [](AsyncWebServerRequest *request) {
        processCalibration(request);
    });
    server.on("/json/", HTTP_GET, [](AsyncWebServerRequest *request) {
        http_json(request);
    });
    server.on("/settings/json/", HTTP_GET, [](AsyncWebServerRequest *request) {
        settings_json(request);
    });
    // About Page Info Handlers
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

#ifndef DISABLE_OTA_UPDATES
    server.on("/ota/", HTTP_GET, [](AsyncWebServerRequest *request) {
        trigger_OTA(request);
    });
#endif
    server.on("/wifi/", HTTP_GET, [](AsyncWebServerRequest *request) {
        trigger_wifi_reset(request);
    });
    server.on("/restart/", HTTP_GET, [](AsyncWebServerRequest *request) {
        trigger_restart(request);
    });

#ifdef FSEDIT
    // Setup Filesystem editor
    server.addHandler(new SPIFFSEditor(FILESYSTEM, "admin", "p@ssword"));

    server.on("/edit/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("/edit");
    });
#endif

    // File not found handler
    server.onNotFound([](AsyncWebServerRequest *request) {
        if (request->method() == HTTP_OPTIONS)
        {
            request->send(200);
        }
        else
        {
            Log.verbose(F("Serving 404 for request to %s." CR), request->url().c_str());
            request->redirect("/404/");
        }
    });

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

    server.begin();
    Log.notice(F("Async HTTP server started." CR));
    Log.verbose(F("Open: http://%s.local to view application." CR), WiFi.getHostname());
}
