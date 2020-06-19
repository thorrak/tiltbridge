//
// Created by John Beeler on 2/17/19.
//

#include <nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;


#include "tiltBridge.h"
#include "wifi_setup.h"

#include "http_server.h"


#include "tilt/tiltScanner.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include "SPIFFS.h"

#include <fstream>
#include <string>
#include <iostream>

#include "OTAUpdate.h"
#include "sendData.h"

httpServer http_server;

WebServer server(80);

void trigger_restart();

inline bool isInteger(const char* s)
{
    // TODO - Fix this
    if(strlen(s) <= 0 || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) return false;

    char * p;
    strtol(s, &p, 10);

    return (*p == 0);
}

// This is to simplify the redirects in processConfig
void redirectToConfig() {
    server.sendHeader("Location", "/settings/");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
}

void processConfigError() {
#ifdef DEBUG_PRINTS
    Serial.println("processConfigError!");
#endif
    redirectToConfig();
}

// This is to simplify the redirects in processCalibration
void redirectToCalibration() {
    server.sendHeader("Location", "/calibration/");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
}

void processCalibrationError() {
#ifdef DEBUG_PRINTS
    Serial.println("processCalibrationError!");
#endif
    redirectToCalibration();
}


bool processSheetName(const char* varName, const char* colorName) {
    if (server.hasArg(varName)) {
        if (server.arg(varName).length() > 64) {
            processConfigError();
            return false;
        } else if (server.arg(varName).length() < 1) {
            app_config.config[varName] = "";
            return true;
        } else {
            app_config.config[varName] = server.arg(varName).c_str();
            return true;
        }
    }
    // True or false is error state - not if it was processed
    return true;
}

void processConfig() {
    bool restart_tiltbridge = false;

    // Generic TiltBridge Settings
    if (server.hasArg("mdnsID")) {
        if (server.arg("mdnsID").length() > 30)
            return processConfigError();
        else if (server.arg("mdnsID").length() < 3)
            return processConfigError();
        else {
            app_config.config["mdnsID"] = server.arg("mdnsID").c_str();

            // When we update the mDNS ID, a lot of things have to get reset. Rather than doing the hard work of actually
            // resetting those settings & broadcasting the new ID, let's just restart the controller.
            restart_tiltbridge = true;
        }
    }

    if (server.hasArg("applyCalibration")) {
        app_config.config["applyCalibration"] = true;
    } else {
        app_config.config["applyCalibration"] = false;
    }

    if (server.hasArg("tempCorrect")) {
        app_config.config["tempCorrect"] = true;
    } else {
        app_config.config["tempCorrect"] = false;
    }

    // Fermentrack Settings
    if (server.hasArg("fermentrackURL")) {
        // TODO - Add a check here to make sure that fermentrackURL actually changed, and return if it didn't
        if (server.arg("fermentrackURL").length() > 255)
            return processConfigError();
        else if (server.arg("fermentrackURL").length() < 12)
            app_config.config["fermentrackURL"] = "";
        else
            app_config.config["fermentrackURL"] = server.arg("fermentrackURL").c_str();
    }

    if (server.hasArg("fermentrackPushEvery")) {
        Serial.println("Has fermentrackPushEvery");
        if (server.arg("fermentrackPushEvery").length() > 5)
            return processConfigError();
        else if (server.arg("fermentrackPushEvery").length() <= 0)
            return processConfigError();
        else if (!isInteger(server.arg("fermentrackPushEvery").c_str())) {
            Serial.println("fermentrackPushEvery is not an integer!");
            return processConfigError();
        }

        // At this point, we know that it's an integer. Let's convert to a long so we can test the value
        // TODO - Figure out if we want to print error messages for these
        long push_every = strtol(server.arg("fermentrackPushEvery").c_str(), nullptr, 10);
        if(push_every < 30)
            app_config.config["fermentrackPushEvery"] = 30;
        else if(push_every > 60*60)
            app_config.config["fermentrackPushEvery"] = 60*60;
        else
            app_config.config["fermentrackPushEvery"] = push_every;
        Serial.println("Updated fermentrackPushEvery");
    }

    // Brewstatus Settings
    if (server.hasArg("brewstatusURL")) {
        // TODO - Add a check here to make sure that brewstatusURL actually changed, and return if it didn't
        if (server.arg("brewstatusURL").length() > 255)
            return processConfigError();
        else if (server.arg("brewstatusURL").length() < 12)
            app_config.config["brewstatusURL"] = "";
        else
            app_config.config["brewstatusURL"] = server.arg("brewstatusURL").c_str();
    }

    if (server.hasArg("brewstatusPushEvery")) {
        Serial.println("Has brewstatusPushEvery");
        if (server.arg("brewstatusPushEvery").length() > 5)
            return processConfigError();
        else if (server.arg("brewstatusPushEvery").length() <= 0)
            return processConfigError();
        else if (!isInteger(server.arg("brewstatusPushEvery").c_str())) {
            Serial.println("brewstatusPushEvery is not an integer!");
            return processConfigError();
        }

        // At this point, we know that it's an integer. Let's convert to a long so we can test the value
        // TODO - Figure out if we want to print error messages for these
        long push_every = strtol(server.arg("brewstatusPushEvery").c_str(), nullptr, 10);
        if(push_every < 30)
            app_config.config["brewstatusPushEvery"] = 30;
        else if(push_every > 60*60)
            app_config.config["brewstatusPushEvery"] = 60*60;
        else
            app_config.config["brewstatusPushEvery"] = push_every;
        Serial.println("Updated brewstatusPushEvery");
    }

    if (server.hasArg("brewstatusTZoffset")) {
        Serial.println("Has brewstatusTZoffset");
        if (server.arg("brewstatusTZoffset").length() > 3)
            return processConfigError();
        else if (server.arg("brewstatusTZoffset").length() <= 0)
            return processConfigError();

        float tzoffset = strtof(server.arg("brewstatusTZoffset").c_str(), nullptr);
        if(tzoffset < -12.0) {
            Serial.println("brewstatusTZoffset is less than -12!");
            return processConfigError();
        } else if(tzoffset > 12.0) {
            Serial.println("brewstatusTZoffset is greater than 12!");
            return processConfigError();
        } else {
            app_config.config["brewstatusTZoffset"] = tzoffset;
        }
        Serial.println("Updated brewstatusTZoffset");
    }

    // Google Sheets Settings
    if (server.hasArg("scriptsURL")) {
        // TODO - Validate this begins with "https://scripts.google.com/"
        if (server.arg("scriptsURL").length() > 255)
            return processConfigError();
        else if (server.arg("scriptsURL").length() < 12)
            app_config.config["scriptsURL"] = "";
        else
            app_config.config["scriptsURL"] = server.arg("scriptsURL").c_str();
    }

    if (server.hasArg("scriptsEmail")) {
        if (server.arg("scriptsEmail").length() > 255)
            return processConfigError();
        else if (server.arg("scriptsEmail").length() < 7)
            app_config.config["scriptsEmail"] = "";
        else
            app_config.config["scriptsEmail"] = server.arg("scriptsEmail").c_str();
    }


    // Individual Google Sheets Beer Log Names
    if (!processSheetName("sheetName_red", "Red"))
        return;
    if (!processSheetName("sheetName_green", "Green"))
        return;
    if (!processSheetName("sheetName_black", "Black"))
        return;
    if (!processSheetName("sheetName_purple", "Purple"))
        return;
    if (!processSheetName("sheetName_orange", "Orange"))
        return;
    if (!processSheetName("sheetName_blue", "Blue"))
        return;
    if (!processSheetName("sheetName_yellow", "Yellow"))
        return;
    if (!processSheetName("sheetName_pink", "Pink"))
        return;

    // Brewers Friend Setting
    if (server.hasArg("brewersFriendKey")) {
        if (server.arg("brewersFriendKey").length() > 255)
            return processConfigError();
        else if (server.arg("brewersFriendKey").length() <= BREWERS_FRIEND_MIN_KEY_LENGTH)
            app_config.config["brewersFriendKey"] = "";
        else
            app_config.config["brewersFriendKey"] = server.arg("brewersFriendKey").c_str();
    }

    // Brewfather
    if (server.hasArg("brewfatherKey")) {
        if (server.arg("brewfatherKey").length() > 255)
            return processConfigError();
        else if (server.arg("brewfatherKey").length() <= BREWFATHER_MIN_KEY_LENGTH)
            app_config.config["brewfatherKey"] = "";
        else
            app_config.config["brewfatherKey"] = server.arg("brewfatherKey").c_str();
    }    

    // If we made it this far, one or more settings were updated. Save.
    app_config.save();

    if(restart_tiltbridge) {
        trigger_restart();
    } else {
        redirectToConfig();
    }
}

constexpr unsigned int str2int(const char* str, int h = 0)
{
    return !str[h] ? 5381 : (str2int(str, h+1) * 33) ^ str[h];
}

// we don't need to do much input checking on the calibration data as we are
// looking at numbers generated by the javascript and not a human
void processCalibration() {
    int tilt_name = 0;
    int degree;
    double x0, x1, x2, x3;

    if (server.hasArg("clearTiltColor")) {
        tilt_name = str2int(server.arg("clearTiltColor").c_str());
        degree = 1;
        x1 = 1.0;
        x0 = x2 = x3 = 0.0;
    }

    if (server.hasArg("updateTiltColor")) {
        tilt_name = str2int(server.arg("updateTiltColor").c_str());
        if (server.hasArg("linear")) {
           degree = 1;
           x0 = strtod(server.arg("linearFitx0").c_str(), nullptr);
           x1 = strtod(server.arg("linearFitx1").c_str(), nullptr);
           x2 = x3 = 0.0;
        } else if (server.hasArg("quadratic")) {
           degree = 2;
           x0 = strtod(server.arg("quadraticFitx0").c_str(), nullptr);
           x1 = strtod(server.arg("quadraticFitx1").c_str(), nullptr);
           x2 = strtod(server.arg("quadraticFitx2").c_str(), nullptr);
           x3 = 0.0;
        } else if (server.hasArg("cubic")) {
           degree = 2;
           x0 = strtod(server.arg("cubicFitx0").c_str(), nullptr);
           x1 = strtod(server.arg("cubicFitx1").c_str(), nullptr);
           x2 = strtod(server.arg("cubicFitx2").c_str(), nullptr);
           x3 = strtod(server.arg("cubicFitx3").c_str(), nullptr);
        } else {
            processCalibrationError();
        }
    }

    switch( tilt_name ) {
        case str2int("red"):
            app_config.config["cal_red"] =  {{"degree", degree}, {"x0", x0}, {"x1", x1}, {"x2", x2}, {"x3", x3}};
            break;
        case str2int("green"):
            app_config.config["cal_green"] =  {{"degree", degree}, {"x0", x0}, {"x1", x1}, {"x2", x2}, {"x3", x3}};
            break;
        case str2int("black"):
            app_config.config["cal_black"] =  {{"degree", degree}, {"x0", x0}, {"x1", x1}, {"x2", x2}, {"x3", x3}};
            break;
        case str2int("purple"):
            app_config.config["cal_purple"] =  {{"degree", degree}, {"x0", x0}, {"x1", x1}, {"x2", x2}, {"x3", x3}};
            break;
        case str2int("orange"):
            app_config.config["cal_orange"] =  {{"degree", degree}, {"x0", x0}, {"x1", x1}, {"x2", x2}, {"x3", x3}};
            break;
        case str2int("blue"):
            app_config.config["cal_blue"] =  {{"degree", degree}, {"x0", x0}, {"x1", x1}, {"x2", x2}, {"x3", x3}};
            break;
        case str2int("yellow"):
            app_config.config["cal_yellow"] =  {{"degree", degree}, {"x0", x0}, {"x1", x1}, {"x2", x2}, {"x3", x3}};
            break;
        case str2int("pink"):
            app_config.config["cal_pink"] =  {{"degree", degree}, {"x0", x0}, {"x1", x1}, {"x2", x2}, {"x3", x3}};
            break;
         default:
             processCalibrationError();
     }

    redirectToCalibration();
}


//This function is overkill for how we're handling things, but
bool loadFromSpiffs(String path)
{
    String dataType = "text/plain";

    if (path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
    else if (path.endsWith(".htm")) dataType = "text/html";
    else if (path.endsWith(".css")) dataType = "text/css";
    else if (path.endsWith(".js")) dataType = "application/javascript";
    else if (path.endsWith(".png")) dataType = "image/png";
    else if (path.endsWith(".gif")) dataType = "image/gif";
    else if (path.endsWith(".jpg")) dataType = "image/jpeg";
    else if (path.endsWith(".ico")) dataType = "image/x-icon";
    else if (path.endsWith(".xml")) dataType = "text/xml";
    else if (path.endsWith(".pdf")) dataType = "application/pdf";
    else if (path.endsWith(".zip")) dataType = "application/zip";

    File dataFile = SPIFFS.open(path.c_str(), "r");   //open file to read
    if (!dataFile)  //unsuccessful open
        return false;
    if (server.hasArg("download")) dataType = "application/octet-stream";
    if (server.streamFile(dataFile, dataType) != dataFile.size()) {}    //a lot happening here

    dataFile.close();

    return true; //shouldn't always return true, Added false above
}
//-----------------------------------------------------------------------------------------

void root_from_spiffs() {
    loadFromSpiffs("/index.htm");
}
void settings_from_spiffs() {
    loadFromSpiffs("/settings.htm");
}
void calibration_from_spiffs() {
    loadFromSpiffs("/calibration.htm");
}
void about_from_spiffs() {
    loadFromSpiffs("/about.htm");
}
void favicon_from_spiffs() {
    loadFromSpiffs("/favicon.ico");
}

#ifndef DISABLE_OTA_UPDATES
void trigger_OTA() {
    loadFromSpiffs("/updating.htm");    // Send a message to the user to let them know what is going on
    app_config.config["update_spiffs"] = true;
    lcd.display_ota_update_screen();    // Trigger this here while everything else is waiting.
    delay(1000);                        // Wait 1 second to let everything send
    tilt_scanner.wait_until_scan_complete();    // Wait for scans to complete (we don't want any tasks running in the background)
    execOTA();                          // Trigger the OTA update
}
#endif

void trigger_wifi_reset() {
    loadFromSpiffs("/wifi_reset.htm");    // Send a message to the user to let them know what is going on
    delay(1000);                                // Wait 1 second to let everything send
    tilt_scanner.wait_until_scan_complete();    // Wait for scans to complete (we don't want any tasks running in the background)
    disconnect_from_wifi_and_restart();         // Reset the wifi settings
}

void trigger_restart() {
    loadFromSpiffs("/restarting.htm");    // Send a message to the user to let them know what is going on
    delay(1000);                                // Wait 1 second to let everything send
    tilt_scanner.wait_until_scan_complete();    // Wait for scans to complete (we don't want any tasks running in the background)
    ESP.restart();         // Restart the TiltBridge
}

void http_json() {
    // I probably don't want this inline so that I can add the Allow-Origin header (in case anyone wants to build
    // scripts that pull this data)
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", tilt_scanner.tilt_to_json().dump().c_str());
}

// settings_json is intended to be used to build the "Change Settings" page
void settings_json() {
    // Not sure if I want to leave allow-origin here, but for now it's OK.
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", app_config.config.dump().c_str());
}


void handleNotFound() {
    String message = "File Not Found\n\n";
    server.send(404, "text/plain", message);
}


void httpServer::init(){
    server.on("/", root_from_spiffs);
    server.on("/about/", about_from_spiffs);
    server.on("/settings/", settings_from_spiffs);
    server.on("/settings/update/", processConfig);

    server.on("/calibration/", calibration_from_spiffs);
    server.on("/calibration/update/", processCalibration);

    server.on("/json/", http_json);
    server.on("/settings/json/", settings_json);
#ifndef DISABLE_OTA_UPDATES
    server.on("/ota/", trigger_OTA);
#endif
    server.on("/wifi/", trigger_wifi_reset);
    server.on("/restart/", trigger_restart);
    server.on("/favicon.ico", favicon_from_spiffs);

    server.onNotFound(handleNotFound);
    server.begin();
}

void httpServer::handleClient(){
    server.handleClient();
}
