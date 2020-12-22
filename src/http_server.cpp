//
// Created by John Beeler on 2/17/19.
// Modified by Tim Pletcher 31-Oct-2020.
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

void isInteger(const char* s, bool &is_int, int32_t &int_value) {
    if( (strlen(s) <= 0) || (!isdigit(s[0])) ) {
        is_int = false;
    }
    char * p;
    int_value = strtol(s, &p, 10);
    is_int = (*p == 0);
}

bool isvalidAddress(const char* s) {
    //Rudimentary check that the address is of the form aaa.bbb.ccc
    //or 111.222.333.444 and all characters are alphanumeric
    if(strlen(s) > 255){
        return false;
    }
    for (int i=0; i < strlen(s); i++) {
        if (!isalnum(s[i]) && s[i]!='.')
            return false;
    }
    int seg_ct = 0;
    char ts[strlen(s)+1];
    strcpy(ts,s);
    char * item = strtok(ts,".");
    while (item != NULL) {
        ++seg_ct;
        item = strtok(NULL, ".");
    }
    if ((seg_ct == 3) || (seg_ct == 4)) {
        return true;
    }
    else {
        return false;
    }
}

bool isValidmdnsName(const char* mdns_name) {
    if (strlen(mdns_name) > 31 || strlen(mdns_name) < 8 || mdns_name[0] == '-')
        return false;
    for (int i=0; i < strlen(mdns_name); i++) {
        if ( !isalnum(mdns_name[i]) && mdns_name[i] != '-' )
            return false;
    }
    return true;
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
    bool all_settings_valid = true;
    bool reinit_tft = false;
    bool mqtt_broker_update = false;

    // Generic TiltBridge Settings
    if (server.hasArg("mdnsID") && (app_config.config["mdnsID"].get<std::string>() != server.arg("mdnsID").c_str()) ) {
        if (isValidmdnsName(server.arg("mdnsID").c_str())) {
            app_config.config["mdnsID"] = server.arg("mdnsID").c_str();
            // When we update the mDNS ID, a lot of things have to get reset. Rather than doing the hard work of actually
            // resetting those settings & broadcasting the new ID, let's just restart the controller.
            restart_tiltbridge = true;
        } else {
            all_settings_valid = false;
        }
    }

    if (server.hasArg("smoothFactor")) {
        int sf;
        bool is_int;
        isInteger(server.arg("smoothFactor").c_str(),is_int,sf);
        if (is_int && (sf != app_config.config["smoothFactor"].get<int>()) && sf >= 0 && sf <= 99) {
            app_config.config["smoothFactor"] = sf;
        } else {
            all_settings_valid = false;
        }
    }

    if (server.hasArg("tempUnit") && (app_config.config["tempUnit"] != server.arg("tempUnit").c_str()) ) {
        // TODO - Come back and re-integrate this with pletch's work
        app_config.config["tempUnit"] = server.arg("tempUnit").c_str();
    }

    if (server.hasArg("invertTFT")) {
        if((server.arg("invertTFT")=="on") && (!app_config.config["invertTFT"].get<bool>())) {
            app_config.config["invertTFT"] = true;
            reinit_tft = true;
        } else if ((server.arg("invertTFT")=="off") && (app_config.config["invertTFT"].get<bool>())){
            app_config.config["invertTFT"] = false;
            reinit_tft = true;
        }
    }

    if (server.hasArg("applyCalibration")) {
        if((server.arg("applyCalibration")=="on") && (!app_config.config["applyCalibration"].get<bool>())) {
            app_config.config["applyCalibration"] = true;
        } else if ((server.arg("applyCalibration")=="off") && (app_config.config["applyCalibration"].get<bool>())){
            app_config.config["applyCalibration"] = false;
        }
    }

    if (server.hasArg("tempCorrect")) {
        if((server.arg("tempCorrect")=="on") && (!app_config.config["tempCorrect"].get<bool>())) {
            app_config.config["tempCorrect"] = true;
        } else if ((server.arg("tempCorrect")=="off") && (app_config.config["tempCorrect"].get<bool>())){
            app_config.config["tempCorrect"] = false;
        }
    }

    // Fermentrack Settings
    if (server.hasArg("fermentrackURL") &&
       (app_config.config["fermentrackURL"].get<std::string>() != server.arg("fermentrackURL").c_str())) {
        if (server.arg("fermentrackURL").length() <= 255) {
            if (server.arg("fermentrackURL").length() < 12) {
                app_config.config["fermentrackURL"] = "";
            }else{
                app_config.config["fermentrackURL"] = server.arg("fermentrackURL").c_str();
            }
        } else {
            all_settings_valid = false;
        }

    }

    if (server.hasArg("fermentrackPushEvery")) {
        int push_every;
        bool is_int;
        isInteger(server.arg("fermentrackPushEvery").c_str(),is_int,push_every);
        if (is_int && push_every <= 3600 && push_every >= 15) {
            app_config.config["fermentrackPushEvery"] = push_every;
        } else {
            all_settings_valid = false;
        }

    }

    // Brewstatus Settings
    if (server.hasArg("brewstatusURL") &&
            (app_config.config["brewstatusURL"].get<std::string>() != server.arg("brewstatusURL").c_str())) {
        if (server.arg("brewstatusURL").length() <= 255) {
            if (server.arg("brewstatusURL").length() < 12) {
                app_config.config["brewstatusURL"] = "";
            } else {
                app_config.config["brewstatusURL"] = server.arg("brewstatusURL").c_str();
            }
        } else {
            all_settings_valid = false;
        }
    }

    if (server.hasArg("brewstatusPushEvery")) {
        int push_every;
        bool is_int;
        isInteger(server.arg("brewstatusPushEvery").c_str(),is_int,push_every);
        if (is_int && push_every <= 3600 && push_every >= 30) {
            app_config.config["brewstatusPushEvery"] = push_every;
        } else {
            all_settings_valid = false;
        }
    }

    if (server.hasArg("brewstatusTZoffset")) {
        if (server.arg("brewstatusTZoffset").length() > 0 && server.arg("brewstatusTZoffset").length() <= 3 ) {
            float tzoffset = strtof(server.arg("brewstatusTZoffset").c_str(), nullptr);
            if(tzoffset >= -12.0 && tzoffset <= 12.0) {
                app_config.config["brewstatusTZoffset"] = tzoffset;
            } else {
#ifdef DEBUG_PRINTS
                Serial.println(F("brewstatusTZoffset is not between -12 and 12!"));
#endif
                all_settings_valid = false;
            }
        }
    }

    // Google Sheets Settings
    if (server.hasArg("scriptsURL")) {
        if (server.arg("scriptsURL").length() <= 255) {
            if (server.arg("scriptsURL").length() > 12 &&
                (strncmp(server.arg("scriptsURL").c_str(),"https://script.google.com/", 26)==0)) {

            app_config.config["scriptsURL"] = server.arg("scriptsURL").c_str();
            } else {
                app_config.config["scriptsURL"] = "";
            }
        } else {
            all_settings_valid = false;
        }
    }

    if (server.hasArg("scriptsEmail")) {
        if (server.arg("scriptsEmail").length() <= 255) {
            if (server.arg("scriptsEmail").length() < 7) {
                app_config.config["scriptsEmail"] = "";
            } else {
                app_config.config["scriptsEmail"] = server.arg("scriptsEmail").c_str();
            }
        } else {
            all_settings_valid = false;
        }
    }


    // Individual Google Sheets Beer Log Names
    if (!processSheetName("sheetName_red", "Red"))
        all_settings_valid = false;
    if (!processSheetName("sheetName_green", "Green"))
        all_settings_valid = false;
    if (!processSheetName("sheetName_black", "Black"))
        all_settings_valid = false;
    if (!processSheetName("sheetName_purple", "Purple"))
        all_settings_valid = false;
    if (!processSheetName("sheetName_orange", "Orange"))
        all_settings_valid = false;
    if (!processSheetName("sheetName_blue", "Blue"))
        all_settings_valid = false;
    if (!processSheetName("sheetName_yellow", "Yellow"))
        all_settings_valid = false;
    if (!processSheetName("sheetName_pink", "Pink"))
        all_settings_valid = false;

    // Brewers Friend Setting
    if (server.hasArg("brewersFriendKey")) {
        if (server.arg("brewersFriendKey").length() <= 255) {
            if (server.arg("brewersFriendKey").length() < BREWERS_FRIEND_MIN_KEY_LENGTH) {
                app_config.config["brewersFriendKey"] = "";
            }else{
                app_config.config["brewersFriendKey"] = server.arg("brewersFriendKey").c_str();
            }
        } else {
            all_settings_valid = false;
        }
    }

    // Brewfather
    if (server.hasArg("brewfatherKey")) {
        if (server.arg("brewfatherKey").length() <= 255) {
            if (server.arg("brewfatherKey").length() < BREWERS_FRIEND_MIN_KEY_LENGTH) {
                app_config.config["brewfatherKey"] = "";
            }else{
                app_config.config["brewfatherKey"] = server.arg("brewfatherKey").c_str();
            }
        } else {
            all_settings_valid = false;
        }
    }

    // MQTT
    if (server.hasArg("mqttBrokerIP") && (app_config.config["mqttBrokerIP"].get<std::string>() != server.arg("mqttBrokerIP").c_str())) {
        if (isvalidAddress(server.arg("mqttBrokerIP").c_str())){
            app_config.config["mqttBrokerIP"] = server.arg("mqttBrokerIP").c_str();
            mqtt_broker_update = true;
        }
        else {
            app_config.config["mqttBrokerIP"] = "";
        }
    }

    if (server.hasArg("mqttBrokerPort")) {
        int port_number;
        bool is_int;
        isInteger(server.arg("mqttBrokerPort").c_str(),is_int,port_number);
        if (is_int && port_number < 65535 && port_number > 1024) {
            if (app_config.config["mqttBrokerPort"].get<int>() != port_number) {
                app_config.config["mqttBrokerPort"] = port_number;
                mqtt_broker_update = true;
            }
        } else {
            all_settings_valid = false;
        }
    }

    if (server.hasArg("mqttPushEvery")) {
        int push_every;
        bool is_int;
        isInteger(server.arg("mqttPushEvery").c_str(),is_int,push_every);
        if (is_int && push_every <= 3600 && push_every >=15 ) {
            app_config.config["mqttPushEvery"] = push_every;
        } else {
            all_settings_valid = false;
        }
    }


    if (server.hasArg("mqttUsername") && (app_config.config["mqttUsername"].get<std::string>() != server.arg("mqttUsername").c_str())) {
        if (server.arg("mqttUsername").length() <= 50) {
            if (server.arg("mqttUsername").length() <= 0){
                app_config.config["mqttUsername"] = "";
            }else{
                app_config.config["mqttUsername"] = server.arg("mqttUsername").c_str();
            }
            mqtt_broker_update = true;
        } else {
            all_settings_valid = false;
        }
    }

    if (server.hasArg("mqttPassword") && (app_config.config["mqttPassword"].get<std::string>() != server.arg("mqttPassword").c_str())) {
        if (server.arg("mqttPassword").length() <= 128) {
            if (server.arg("mqttPassword").length() <= 0){
                app_config.config["mqttPassword"] = "";
            }else{
            app_config.config["mqttPassword"] = server.arg("mqttPassword").c_str();
            }
            mqtt_broker_update = true;
        } else {
            all_settings_valid = false;
        }
    }


    if (server.hasArg("mqttTopic")) {
        if (server.arg("mqttTopic").length() <= 255){
            if (server.arg("mqttTopic").length() <= 2) {
                app_config.config["mqttTopic"] = "tiltbridge";
            }else{
                app_config.config["mqttTopic"] = server.arg("mqttTopic").c_str();
            }
        } else {
            all_settings_valid = false;
        }
    }

    // If we made it this far, one or more settings were updated. Save.
    if(all_settings_valid) {
        app_config.save();
    } else {
        return processConfigError();
    }

    if(mqtt_broker_update) {
            data_sender.init_mqtt();
    }

    if(reinit_tft) {
        lcd.init();
    }

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


bool loadFromSpiffs(const char* path)
{
    char p[strlen(path)+1];
    strcpy(p,path);
    char * suffix = strtok(p,".");
    suffix = strtok(NULL,".");

    char dataType[25];

    if (strncmp(suffix,"htm",3)==0) strcpy(dataType,"text/html");
    else if (strncmp(suffix,"css",3)==0) strcpy(dataType,"text/css");
    else if (strncmp(suffix,"js",3)==0) strcpy(dataType,"application/javascript");
    else if (strncmp(suffix,"png",3)==0) strcpy(dataType,"image/png");
    else if (strncmp(suffix,"gif",3)==0) strcpy(dataType,"image/gif");
    else if (strncmp(suffix,"jpg",3)==0) strcpy(dataType,"image/jpeg");
    else if (strncmp(suffix,"ico",3)==0) strcpy(dataType,"image/x-icon");
    else if (strncmp(suffix,"xml",3)==0) strcpy(dataType,"text/xml");
    else if (strncmp(suffix,"pdf",3)==0) strcpy(dataType,"application/pdf");
    else if (strncmp(suffix,"zip",3)==0) strcpy(dataType,"application/zip");
    else strcpy(dataType,"text/plain");

    File dataFile = SPIFFS.open(path, "r");   //open file to read
    if (!dataFile)  //unsuccessful open
        return false;
    server.streamFile(dataFile, dataType);
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
    server.send(200, "application/json", tilt_scanner.tilt_to_json(false).dump().c_str());
}

// settings_json is intended to be used to build the "Change Settings" page
void settings_json() {
    // Not sure if I want to leave allow-origin here, but for now it's OK.
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", app_config.config.dump().c_str());
}


void handleNotFound() {
    server.send(404, "text/plain", "File Not Found\n\n");
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
