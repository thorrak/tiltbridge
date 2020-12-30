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

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "SPIFFS.h"

#include <fstream>
#include <string>
#include <iostream>

#include "OTAUpdate.h"
#include "sendData.h"

httpServer http_server;

AsyncWebServer server(80);

char all_valid[2] = "1";

void trigger_restart(AsyncWebServerRequest *request);

void isInteger(const char* s, bool &is_int, int32_t &int_value) {
    if( (strlen(s) <= 0) || (!isdigit(s[0])) ) {
        is_int = false;
    }
    char * p;
    int_value = strtol(s, &p, 10);
    is_int = (*p == 0);
}

bool isvalidAddress(const char* s) {
    //Rudimentary check that the address is of the form aaa.bbb.ccc or
    //aaa.bbb (if DNS) or 111.222.333.444 (if IP). Will not currently catch if integer > 254 
    // is input when using IP address
    if(strlen(s) > 253){
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
    if (( s[0] == '-' ) || ( s[0] == '.' ) || ( s[strlen(s)-1] == '-' ) || ( s[strlen(s)-1] == '.' ))
        return false;
    for (int i=0; i < strlen(s); i++) {
        if (seg_ct == 2 || seg_ct == 3) { //must be a DNS name if true
            if ( !isalnum(s[i]) && s[i] != '.' && s[i] != '-' ) 
                return false;
        } else if (seg_ct == 4) { //must be an IP if true
            if ( !isdigit(s[i]) && s[i] != '.' )
                return false;
        } else {
            return false;
        }
    }
    return true;
}

bool isValidmdnsName(const char* mdns_name) {
    if (strlen(mdns_name) > 31 || strlen(mdns_name) < 8 || mdns_name[0] == '-' || mdns_name[strlen(mdns_name)-1] == '-')
        return false;
    for (int i=0; i < strlen(mdns_name); i++) {
        if ( !isalnum(mdns_name[i]) && mdns_name[i] != '-' )
            return false;
    }
    return true;
}

// This is to simplify the redirects in processConfig
void redirectToConfig(AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(301);
    response->addHeader("Location","/settings/");
    response->addHeader("Cache-Control","no-cache");
    request->send(response);
}

void processConfigError(AsyncWebServerRequest *request) {
#ifdef DEBUG_PRINTS
    Serial.println("processConfigError!");
#endif
    all_valid[0] = '0';
    redirectToConfig(request);
}

// This is to simplify the redirects in processCalibration
void redirectToCalibration(AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(301);
    response->addHeader("Location","/calibration/");
    response->addHeader("Cache-Control","no-cache");
    request->send(response);
}

void processCalibrationError(AsyncWebServerRequest *request) {
#ifdef DEBUG_PRINTS
    Serial.println("processCalibrationError!");
#endif
    redirectToCalibration(request);
}


bool processSheetName(const char* varName, const char* colorName, AsyncWebServerRequest *request) {
    if (request->hasArg(varName)) {
        if (request->arg(varName).length() > 64) {
            return false;
        } else if (request->arg(varName).length() < 1) {
            app_config.config[varName] = "";
            return true;
        } else {
            app_config.config[varName] = request->arg(varName).c_str();
            return true;
        }
    }
    // True or false is error state - not if it was processed
    return true;
}

void processConfig(AsyncWebServerRequest *request) {
    bool restart_tiltbridge = false;
    bool all_settings_valid = true;
    bool reinit_tft = false;
    bool mqtt_broker_update = false;

    // Generic TiltBridge Settings
    if (request->hasArg("mdnsID") && (app_config.config["mdnsID"].get<std::string>() != request->arg("mdnsID").c_str()) ) {
        if (isValidmdnsName(request->arg("mdnsID").c_str())) {
            app_config.config["mdnsID"] = request->arg("mdnsID").c_str();
            // When we update the mDNS ID, a lot of things have to get reset. Rather than doing the hard work of actually
            // resetting those settings & broadcasting the new ID, let's just restart the controller.
            restart_tiltbridge = true;
        } else {
            all_settings_valid = false;
        }
    }

    if (request->hasArg("TZoffset")) {
        if (request->arg("TZoffset").length() > 0 && request->arg("TZoffset").length() <= 3 ) {
            int tzo;
            bool is_int;
            isInteger(request->arg("TZoffset").c_str(),is_int,tzo);
            if(tzo >= -12 && tzo <= 14) {
                app_config.config["TZoffset"] = tzo;
            } else {
#ifdef DEBUG_PRINTS
                Serial.println(F("brewstatusTZoffset is not between -11 and 12!"));
#endif
                all_settings_valid = false;
            }
        }
    }

    if (request->hasArg("smoothFactor")) {
        int sf;
        bool is_int;
        isInteger(request->arg("smoothFactor").c_str(),is_int,sf);
        if (sf != app_config.config["smoothFactor"].get<int>()) {
            if (is_int && sf >= 0 && sf <= 99) {
                app_config.config["smoothFactor"] = sf;
            } else {
                all_settings_valid = false;
            }
        }
    }

    if (request->hasArg("tempUnit") && (app_config.config["tempUnit"] != request->arg("tempUnit").c_str()) ) {
        // TODO - Come back and re-integrate this with pletch's work
        app_config.config["tempUnit"] = request->arg("tempUnit").c_str();
    }

    if (request->hasArg("invertTFT")) {
        if((request->arg("invertTFT")=="on") && (!app_config.config["invertTFT"].get<bool>())) {
            app_config.config["invertTFT"] = true;
            reinit_tft = true;
        } else if ((request->arg("invertTFT")=="off") && (app_config.config["invertTFT"].get<bool>())){
            app_config.config["invertTFT"] = false;
            reinit_tft = true;
        }
    }

    if (request->hasArg("applyCalibration")) {
        if((request->arg("applyCalibration")=="on") && (!app_config.config["applyCalibration"].get<bool>())) {
            app_config.config["applyCalibration"] = true;
        } else if ((request->arg("applyCalibration")=="off") && (app_config.config["applyCalibration"].get<bool>())){
            app_config.config["applyCalibration"] = false;
        }
    }

    if (request->hasArg("tempCorrect")) {
        if((request->arg("tempCorrect")=="on") && (!app_config.config["tempCorrect"].get<bool>())) {
            app_config.config["tempCorrect"] = true;
        } else if ((request->arg("tempCorrect")=="off") && (app_config.config["tempCorrect"].get<bool>())){
            app_config.config["tempCorrect"] = false;
        }
    }

    // Fermentrack Settings
    if (request->hasArg("fermentrackURL") &&
       (app_config.config["fermentrackURL"].get<std::string>() != request->arg("fermentrackURL").c_str())) {
        if (request->arg("fermentrackURL").length() <= 255) {
            if (request->arg("fermentrackURL").length() < 12) {
                app_config.config["fermentrackURL"] = "";
            }else{
                app_config.config["fermentrackURL"] = request->arg("fermentrackURL").c_str();
            }
        } else {
            all_settings_valid = false;
        }

    }

    if (request->hasArg("fermentrackPushEvery")) {
        int push_every;
        bool is_int;
        isInteger(request->arg("fermentrackPushEvery").c_str(),is_int,push_every);
        if (is_int && push_every <= 3600 && push_every >= 15) {
            app_config.config["fermentrackPushEvery"] = push_every;
        } else {
            all_settings_valid = false;
        }

    }

    // Brewstatus Settings
    if (request->hasArg("brewstatusURL") &&
            (app_config.config["brewstatusURL"].get<std::string>() != request->arg("brewstatusURL").c_str())) {
        if (request->arg("brewstatusURL").length() <= 255) {
            if (request->arg("brewstatusURL").length() < 12) {
                app_config.config["brewstatusURL"] = "";
            } else {
                app_config.config["brewstatusURL"] = request->arg("brewstatusURL").c_str();
            }
        } else {
            all_settings_valid = false;
        }
    }

    if (request->hasArg("brewstatusPushEvery")) {
        int push_every;
        bool is_int;
        isInteger(request->arg("brewstatusPushEvery").c_str(),is_int,push_every);
        if (is_int && push_every <= 3600 && push_every >= 30) {
            app_config.config["brewstatusPushEvery"] = push_every;
        } else {
            all_settings_valid = false;
        }
    }

    // Google Sheets Settings
    if (request->hasArg("scriptsURL")) {
        if (request->arg("scriptsURL").length() <= 255) {
            if (request->arg("scriptsURL").length() > 12 &&
                (strncmp(request->arg("scriptsURL").c_str(),"https://script.google.com/", 26)==0)) {

            app_config.config["scriptsURL"] = request->arg("scriptsURL").c_str();
            } else {
                app_config.config["scriptsURL"] = "";
            }
        } else {
            all_settings_valid = false;
        }
    }

    if (request->hasArg("scriptsEmail")) {
        if (request->arg("scriptsEmail").length() <= 255) {
            if (request->arg("scriptsEmail").length() < 7) {
                app_config.config["scriptsEmail"] = "";
            } else {
                app_config.config["scriptsEmail"] = request->arg("scriptsEmail").c_str();
            }
        } else {
            all_settings_valid = false;
        }
    }


    // Individual Google Sheets Beer Log Names
    if (!processSheetName("sheetName_red", "Red", request))
        all_settings_valid = false;
    if (!processSheetName("sheetName_green", "Green", request))
        all_settings_valid = false;
    if (!processSheetName("sheetName_black", "Black", request))
        all_settings_valid = false;
    if (!processSheetName("sheetName_purple", "Purple", request))
        all_settings_valid = false;
    if (!processSheetName("sheetName_orange", "Orange", request))
        all_settings_valid = false;
    if (!processSheetName("sheetName_blue", "Blue", request))
        all_settings_valid = false;
    if (!processSheetName("sheetName_yellow", "Yellow", request))
        all_settings_valid = false;
    if (!processSheetName("sheetName_pink", "Pink", request))
        all_settings_valid = false;

    // Brewers Friend Setting
    if (request->hasArg("brewersFriendKey")) {
        if (request->arg("brewersFriendKey").length() <= 255) {
            if (request->arg("brewersFriendKey").length() < BREWERS_FRIEND_MIN_KEY_LENGTH) {
                app_config.config["brewersFriendKey"] = "";
            }else{
                app_config.config["brewersFriendKey"] = request->arg("brewersFriendKey").c_str();
            }
        } else {
            all_settings_valid = false;
        }
    }

    // Brewfather
    if (request->hasArg("brewfatherKey")) {
        if (request->arg("brewfatherKey").length() <= 255) {
            if (request->arg("brewfatherKey").length() < BREWERS_FRIEND_MIN_KEY_LENGTH) {
                app_config.config["brewfatherKey"] = "";
            }else{
                app_config.config["brewfatherKey"] = request->arg("brewfatherKey").c_str();
            }
        } else {
            all_settings_valid = false;
        }
    }

    // MQTT
    if (request->hasArg("mqttBrokerIP") && (app_config.config["mqttBrokerIP"].get<std::string>() != request->arg("mqttBrokerIP").c_str())) {
        if (isvalidAddress(request->arg("mqttBrokerIP").c_str())){
            app_config.config["mqttBrokerIP"] = request->arg("mqttBrokerIP").c_str();
            mqtt_broker_update = true;
        }
        else if (request->arg("mqttBrokerIP").length() < 2) {
            app_config.config["mqttBrokerIP"] = "";
        }
        else {
            all_settings_valid = false;
        }
    }

    if (request->hasArg("mqttBrokerPort")) {
        int port_number;
        bool is_int;
        isInteger(request->arg("mqttBrokerPort").c_str(),is_int,port_number);
        if (is_int && port_number < 65535 && port_number > 1024) {
            if (app_config.config["mqttBrokerPort"].get<int>() != port_number) {
                app_config.config["mqttBrokerPort"] = port_number;
                mqtt_broker_update = true;
            }
        } else {
            all_settings_valid = false;
        }
    }

    if (request->hasArg("mqttPushEvery")) {
        int push_every;
        bool is_int;
        isInteger(request->arg("mqttPushEvery").c_str(),is_int,push_every);
        if (is_int && push_every <= 3600 && push_every >=15 ) {
            app_config.config["mqttPushEvery"] = push_every;
        } else {
            all_settings_valid = false;
        }
    }


    if (request->hasArg("mqttUsername") && (app_config.config["mqttUsername"].get<std::string>() != request->arg("mqttUsername").c_str())) {
        if (request->arg("mqttUsername").length() <= 50) {
            if (request->arg("mqttUsername").length() <= 0){
                app_config.config["mqttUsername"] = "";
            }else{
                app_config.config["mqttUsername"] = request->arg("mqttUsername").c_str();
            }
            mqtt_broker_update = true;
        } else {
            all_settings_valid = false;
        }
    }

    if (request->hasArg("mqttPassword") && (app_config.config["mqttPassword"].get<std::string>() != request->arg("mqttPassword").c_str())) {
        if (request->arg("mqttPassword").length() <= 128) {
            if (request->arg("mqttPassword").length() <= 0){
                app_config.config["mqttPassword"] = "";
            }else{
            app_config.config["mqttPassword"] = request->arg("mqttPassword").c_str();
            }
            mqtt_broker_update = true;
        } else {
            all_settings_valid = false;
        }
    }


    if (request->hasArg("mqttTopic")) {
        if (request->arg("mqttTopic").length() <= 30){
            if (request->arg("mqttTopic").length() <= 2) {
                app_config.config["mqttTopic"] = "tiltbridge";
            }else{
                app_config.config["mqttTopic"] = request->arg("mqttTopic").c_str();
            }
        } else {
            all_settings_valid = false;
        }
    }

    // If we made it this far, one or more settings were updated. Save.
    if(all_settings_valid) {
        app_config.save();
    } else {
        return processConfigError(request);
    }

    if(mqtt_broker_update) {
            data_sender.init_mqtt();
    }

    if(reinit_tft) {
        lcd.init();
    }

    if(restart_tiltbridge) {
        trigger_restart(request);
    } else {
        redirectToConfig(request);
    }
}

constexpr unsigned int str2int(const char* str, int h = 0)
{
    return !str[h] ? 5381 : (str2int(str, h+1) * 33) ^ str[h];
}

// we don't need to do much input checking on the calibration data as we are
// looking at numbers generated by the javascript and not a human
void processCalibration(AsyncWebServerRequest *request) {
    int tilt_name = 0;
    int degree;
    double x0, x1, x2, x3;

    if (request->hasArg("clearTiltColor")) {
        tilt_name = str2int(request->arg("clearTiltColor").c_str());
        degree = 1;
        x1 = 1.0;
        x0 = x2 = x3 = 0.0;
    }

    if (request->hasArg("updateTiltColor")) {
        tilt_name = str2int(request->arg("updateTiltColor").c_str());
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
             processCalibrationError(request);
     }

    redirectToCalibration(request);
}


bool loadFromSpiffs(const char* path, AsyncWebServerRequest *request)
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
    request->send(SPIFFS,path,dataType);
    dataFile.close();

    return true; //shouldn't always return true, Added false above
}
//-----------------------------------------------------------------------------------------

void root_from_spiffs(AsyncWebServerRequest *request) {
    loadFromSpiffs("/index.htm", request);
}
void settings_from_spiffs(AsyncWebServerRequest *request) {
    loadFromSpiffs("/settings.htm", request);
}
void calibration_from_spiffs(AsyncWebServerRequest *request) {
    loadFromSpiffs("/calibration.htm", request);
}
void about_from_spiffs(AsyncWebServerRequest *request) {
    loadFromSpiffs("/about.htm", request);
}
void favicon_from_spiffs(AsyncWebServerRequest *request) {
    loadFromSpiffs("/favicon.ico", request);
}

#ifndef DISABLE_OTA_UPDATES
void trigger_OTA(syncWebServerRequest *request) {
    loadFromSpiffs("/updating.htm", request);    // Send a message to the user to let them know what is going on
    app_config.config["update_spiffs"] = true;
    lcd.display_ota_update_screen();    // Trigger this here while everything else is waiting.
    delay(1000);                        // Wait 1 second to let everything send
    tilt_scanner.wait_until_scan_complete();    // Wait for scans to complete (we don't want any tasks running in the background)
    execOTA();                          // Trigger the OTA update
}
#endif

void trigger_wifi_reset(AsyncWebServerRequest *request) {
    loadFromSpiffs("/wifi_reset.htm",request);    // Send a message to the user to let them know what is going on
    delay(1000);                                // Wait 1 second to let everything send
    tilt_scanner.wait_until_scan_complete();    // Wait for scans to complete (we don't want any tasks running in the background)
    disconnect_from_wifi_and_restart();         // Reset the wifi settings
}

void trigger_restart(AsyncWebServerRequest *request) {
    loadFromSpiffs("/restarting.htm", request);    // Send a message to the user to let them know what is going on
    http_server.restart_requested = true;
}

void http_json(AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200,"application/json",tilt_scanner.tilt_to_json(false).dump().c_str());
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

// settings_json is intended to be used to build the "Change Settings" page
void settings_json(AsyncWebServerRequest *request) {
    // Not sure if I want to leave allow-origin here, but for now it's OK.
    //
    // Code modified to bundle an all_valid setting on end of json string to allow
    // javascript code to determine if a bad config value was passed.
    // Seems like there should be a cleaner way to do this but it works for now.
    char json_string[strlen(app_config.config.dump().c_str())+16];
    json_string[0] = {'\0'};
    strncat(json_string,app_config.config.dump().c_str(),strlen(app_config.config.dump().c_str())-1);
    strcat(json_string,",\"all_valid\":");
    strcat(json_string,all_valid);
    strcat(json_string,"}");
    AsyncWebServerResponse *response = request->beginResponse(200,"application/json",json_string);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
    all_valid[0] = '1';
}


void handleNotFound(AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(404,"text/plain","File Not Found\n\n");
    request->send(response);
}



void httpServer::init(){
    server.on("/",HTTP_GET,[](AsyncWebServerRequest *request){
        root_from_spiffs(request);
    });
    server.on("/about/",HTTP_GET,[](AsyncWebServerRequest *request){
        about_from_spiffs(request);
    });
    server.on("/settings/",HTTP_GET,[](AsyncWebServerRequest *request){
        settings_from_spiffs(request);
    });
    server.on("/settings/update/",HTTP_POST,[](AsyncWebServerRequest *request){
        processConfig(request);
    });
    server.on("/calibration/",HTTP_GET,[](AsyncWebServerRequest *request){
        calibration_from_spiffs(request);
    });
    server.on("/calibration/update/",HTTP_POST,[](AsyncWebServerRequest *request){
        processCalibration(request);
    });
    server.on("/json/",HTTP_GET,[](AsyncWebServerRequest *request){
        http_json(request);
    });
    server.on("/settings/json/",HTTP_GET,[](AsyncWebServerRequest *request){
        settings_json(request);
    });
#ifndef DISABLE_OTA_UPDATES
    server.on("/ota/",HTTP_GET,[](AsyncWebServerRequest *request){
        trigger_OTA(request);
    });
#endif
    server.on("/wifi/",HTTP_GET,[](AsyncWebServerRequest *request){
        trigger_wifi_reset(request);
    });
    server.on("/restart/",HTTP_GET,[](AsyncWebServerRequest *request){
        trigger_restart(request);
    });
    server.on("/favicon.ico",HTTP_GET,[](AsyncWebServerRequest *request){
        favicon_from_spiffs(request);
    });

    server.onNotFound(handleNotFound);
    server.begin();
}
