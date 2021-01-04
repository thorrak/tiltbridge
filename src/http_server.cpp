//
// Created by John Beeler on 2/17/19.
// Modified by Tim Pletcher 31-Oct-2020.
//

//#include <nlohmann/json.hpp>
#include "resetreasons.h"

// for convenience
//using json = nlohmann::json;


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

#define FILESYSTEM SPIFFS

#if FILESYSTEM == SPIFFS
#include "SPIFFS.h"
#endif

#define DBG_OUTPUT_PORT Serial

//void trigger_restart();
void trigger_restart(AsyncWebServerRequest *request);

bool exists(String path)
{
    bool yes = false;
    File file = FILESYSTEM.open(path, "r");
    if (!file.isDirectory())
    {
        yes = true;
    }
    file.close();
    return yes;
}

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
    DBG_OUTPUT_PORT.println("processConfigError!");
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
    DBG_OUTPUT_PORT.println("processCalibrationError!");
#endif
    redirectToCalibration(request);
}


void processSheetName(const char* varName, bool &is_empty, bool &is_valid, AsyncWebServerRequest *request) {
    is_valid = false;
    is_empty = false;
    if (request->hasArg(varName)) {
        if (request->arg(varName).length() > 64) {
            is_empty = false;
            is_valid = false;
        } else if (request->arg(varName).length() < 1) {
            is_empty = true;
            is_valid = true;
        } else {
            is_valid = true;
            is_empty = false;
        }
    }
    // True or false is error state - not if it was processed
}

void processConfig(AsyncWebServerRequest *request) {
    bool restart_tiltbridge = false;
    bool all_settings_valid = true;
    bool reinit_tft = false;
    bool mqtt_broker_update = false;

    // Generic TiltBridge Settings
    if (request->hasArg("mdnsID") ) {
        if (strncmp(app_config.config.mdnsID,request->arg("mdnsID").c_str(),32) != 0) {
            if (isValidmdnsName(request->arg("mdnsID").c_str())) {
                strlcpy(app_config.config.mdnsID, request->arg("mdnsID").c_str(), 32);
                // When we update the mDNS ID, a lot of things have to get reset. Rather than doing the hard work of actually
                // resetting those settings & broadcasting the new ID, let's just restart the controller.
                restart_tiltbridge = true;
            } else {
                all_settings_valid = false;
            }
        }
        if (request->arg("TZoffset").length() > 0 && request->arg("TZoffset").length() <= 3 ) {
            int tzo;
            bool is_int;
            isInteger(request->arg("TZoffset").c_str(),is_int,tzo);
            if(tzo >= -12 && tzo <= 14) {
                app_config.config.TZoffset = tzo;
            } else {
#ifdef DEBUG_PRINTS
                DBG_OUTPUT_PORT.println(F("brewstatusTZoffset is not between -12 and 14!"));
#endif
                all_settings_valid = false;
            }
        }

        int sf;
        bool is_int;
        isInteger(request->arg("smoothFactor").c_str(),is_int,sf);
        if (sf != app_config.config.smoothFactor) {
            if (is_int && sf >= 0 && sf <= 99) {
                app_config.config.smoothFactor = sf;
            } else {
                all_settings_valid = false;
            }
        }

        strlcpy(app_config.config.tempUnit,request->arg("tempUnit").c_str(),2);

        if( request->arg("invertTFT")=="on" && !app_config.config.invertTFT ) {
            app_config.config.invertTFT = true;
            reinit_tft = true;
        } else if ( request->arg("invertTFT")=="off" && app_config.config.invertTFT ){
            app_config.config.invertTFT = false;
            reinit_tft = true;
        }
    }

    if (request->hasArg("applyCalibration")) {
        if( request->arg("applyCalibration")=="on" &&  !app_config.config.applyCalibration ) {
            app_config.config.applyCalibration = true;
        } else if ( request->arg("applyCalibration") == "off" && app_config.config.applyCalibration ){
            app_config.config.applyCalibration = false;
        }

        if( request->arg("tempCorrect") == "on" && !app_config.config.tempCorrect ) {
            app_config.config.tempCorrect = true;
        } else if ( request->arg("tempCorrect")=="off" && app_config.config.tempCorrect ) {
            app_config.config.tempCorrect = false;
        }
    
    }

    // LocalTarget Settings
    if (request->hasArg("localTargetURL")) {
        if (request->arg("localTargetURL").length() <= 255) {
            if (request->arg("localTargetURL").length() < 12) {
                strlcpy(app_config.config.localTargetURL,"",2);
            }else{
                strlcpy(app_config.config.localTargetURL,request->arg("localTargetURL").c_str(),256);
            }
        } else {
            all_settings_valid = false;
        }

        int push_every;
        bool is_int;
        isInteger(request->arg("localTargetPushEvery").c_str(),is_int,push_every);
        if (is_int && push_every <= 3600 && push_every >= 15) {
            app_config.config.localTargetPushEvery = push_every;
        } else {
            all_settings_valid = false;
        }

    }

    // Brewstatus Settings
    if (request->hasArg("brewstatusURL")) {
        if (request->arg("brewstatusURL").length() <= 255) {
            if (request->arg("brewstatusURL").length() < 12) {
                strlcpy(app_config.config.brewstatusURL,"",2);
            } else {
                strlcpy(app_config.config.brewstatusURL, request->arg("brewstatusURL").c_str(),256);
            }
        } else {
            all_settings_valid = false;
        }

        int push_every;
        bool is_int;
        isInteger(request->arg("brewstatusPushEvery").c_str(),is_int,push_every);
        if (is_int && push_every <= 3600 && push_every >= 30) {
            app_config.config.brewstatusPushEvery = push_every;
        } else {
            all_settings_valid = false;
        }
    }

    // Google Sheets Settings
    if (request->hasArg("scriptsURL")) {
        if (request->arg("scriptsURL").length() <= 255) {
            if (request->arg("scriptsURL").length() > 12 &&
                (strncmp(request->arg("scriptsURL").c_str(),"https://script.google.com/", 26)==0)) {
                strlcpy(app_config.config.scriptsURL,request->arg("scriptsURL").c_str(),256);
            } else {
                strlcpy(app_config.config.scriptsURL,"",2);
            }
        } else {
            all_settings_valid = false;
        }

        if (request->arg("scriptsEmail").length() <= 255) {
            if (request->arg("scriptsEmail").length() < 7) {
                strlcpy(app_config.config.scriptsEmail,"",2);
            } else {
                strlcpy(app_config.config.scriptsEmail,request->arg("scriptsEmail").c_str(),256);
            }
        } else {
            all_settings_valid = false;
        }

    // Individual Google Sheets Beer Log Names
        bool is_empty;
        bool is_valid;
        processSheetName("sheetName_red", is_empty,is_valid,request);
        if (!is_valid) {
            all_settings_valid = false;
        } else if (is_empty) {
            strlcpy(app_config.config.sheetName_red,"",2);
        } else {
            strlcpy(app_config.config.sheetName_red,request->arg("sheetName_red").c_str(),25);
        }
        processSheetName("sheetName_green", is_empty,is_valid,request);
        if (!is_valid) {
            all_settings_valid = false;
        } else if (is_empty) {
            strlcpy(app_config.config.sheetName_green,"",2);
        } else {
            strlcpy(app_config.config.sheetName_green,request->arg("sheetName_green").c_str(),25);
        }
        processSheetName("sheetName_black", is_empty,is_valid,request);
        if (!is_valid) {
            all_settings_valid = false;
        } else if (is_empty) {
            strlcpy(app_config.config.sheetName_black,"",2);
        } else {
            strlcpy(app_config.config.sheetName_black,request->arg("sheetName_black").c_str(),25);
        }
        processSheetName("sheetName_purple", is_empty,is_valid,request);
        if (!is_valid) {
            all_settings_valid = false;
        } else if (is_empty) {
            strlcpy(app_config.config.sheetName_purple,"",2);
        } else {
            strlcpy(app_config.config.sheetName_purple,request->arg("sheetName_purple").c_str(),25);
        }
        processSheetName("sheetName_orange", is_empty,is_valid,request);
        if (!is_valid) {
            all_settings_valid = false;
        } else if (is_empty) {
            strlcpy(app_config.config.sheetName_orange,"",2);
        } else {
            strlcpy(app_config.config.sheetName_orange,request->arg("sheetName_orange").c_str(),25);
        }
        processSheetName("sheetName_blue", is_empty,is_valid,request);
        if (!is_valid) {
            all_settings_valid = false;
        } else if (is_empty) {
            strlcpy(app_config.config.sheetName_blue,"",2);
        } else {
            strlcpy(app_config.config.sheetName_blue,request->arg("sheetName_blue").c_str(),25);
        }
        processSheetName("sheetName_yellow", is_empty,is_valid,request);
        if (!is_valid) {
            all_settings_valid = false;
        } else if (is_empty) {
            strlcpy(app_config.config.sheetName_yellow,"",2);
        } else {
            strlcpy(app_config.config.sheetName_yellow,request->arg("sheetName_yellow").c_str(),25);
        }
        processSheetName("sheetName_pink", is_empty,is_valid,request);
        if (!is_valid) {
            all_settings_valid = false;
        } else if (is_empty) {
            strlcpy(app_config.config.sheetName_pink,"",2);
        } else {
            strlcpy(app_config.config.sheetName_pink,request->arg("sheetName_pink").c_str(),25);
        }
    }
    // Brewers Friend Setting
    if (request->hasArg("brewersFriendKey")) {
        if (request->arg("brewersFriendKey").length() <= 255) {
            if (request->arg("brewersFriendKey").length() < BREWERS_FRIEND_MIN_KEY_LENGTH) {
                strlcpy(app_config.config.brewersFriendKey,"",2);
            }else{
                strlcpy(app_config.config.brewersFriendKey,request->arg("brewersFriendKey").c_str(),25);
            }
        } else {
            all_settings_valid = false;
        }
    }

    // Brewfather
    if (request->hasArg("brewfatherKey")) {
        if (request->arg("brewfatherKey").length() <= 255) {
            if (request->arg("brewfatherKey").length() < BREWERS_FRIEND_MIN_KEY_LENGTH) {
                strlcpy(app_config.config.brewfatherKey,"",2);
            }else{
                strlcpy(app_config.config.brewfatherKey,request->arg("brewfatherKey").c_str(),25);
            }
        } else {
            all_settings_valid = false;
        }
    }

    // MQTT
    if (request->hasArg("mqttBrokerIP")) {
        if (isvalidAddress(request->arg("mqttBrokerIP").c_str())){
            strlcpy(app_config.config.mqttBrokerIP,request->arg("mqttBrokerIP").c_str(),254);
            mqtt_broker_update = true;
        }
        else if (request->arg("mqttBrokerIP").length() < 2) {
            strlcpy(app_config.config.mqttBrokerIP,"",2);
        }
        else {
            all_settings_valid = false;
        }
    
        int port_number;
        bool is_int;
        isInteger(request->arg("mqttBrokerPort").c_str(),is_int,port_number);
        if (is_int && port_number < 65535 && port_number > 1024) {
            if (app_config.config.mqttBrokerPort != port_number) {
                app_config.config.mqttBrokerPort = port_number;
                mqtt_broker_update = true;
            }
        } else {
            all_settings_valid = false;
        }

        int push_every;
        isInteger(request->arg("mqttPushEvery").c_str(),is_int,push_every);
        if (is_int && push_every <= 3600 && push_every >=15 ) {
            app_config.config.mqttPushEvery = push_every;
        } else {
            all_settings_valid = false;
        }

        if (request->arg("mqttUsername").length() <= 50) {
            if (request->arg("mqttUsername").length() <= 0){
                strlcpy(app_config.config.mqttUsername,"",2);
            }else{
                strlcpy(app_config.config.mqttUsername,request->arg("mqttUsername").c_str(),51);
            }
            mqtt_broker_update = true;
        } else {
            all_settings_valid = false;
        }

        if (request->arg("mqttPassword").length() <= 128) {
            if (request->arg("mqttPassword").length() <= 0){
                strlcpy(app_config.config.mqttPassword,"",2);
            }else{
                strlcpy(app_config.config.mqttPassword,request->arg("mqttPassword").c_str(),65);
            }
            mqtt_broker_update = true;
        } else {
            all_settings_valid = false;
        }

        if (request->arg("mqttTopic").length() <= 30){
            if (request->arg("mqttTopic").length() <= 2) {
                strlcpy(app_config.config.mqttTopic,"tiltbridge",11);
            }else{
                strlcpy(app_config.config.mqttTopic,request->arg("mqttTopic").c_str(),31);
            }
        } else {
            all_settings_valid = false;
        }
    }

    

    // If we made it this far, one or more settings were updated. Save.
    if(all_settings_valid) {
        http_server.config_updated = true;
    } else {
        return processConfigError(request);
    }

    if(mqtt_broker_update) {
        http_server.mqtt_init_rqd = true;
    }

    if(reinit_tft) {
        http_server.lcd_init_rqd = true;
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
            app_config.config.cal_red_degree = degree;
            app_config.config.cal_red_x0 = x0;
            app_config.config.cal_red_x1 = x1;
            app_config.config.cal_red_x2 = x2;
            app_config.config.cal_red_x3 = x3;
            break;
        case str2int("green"):
            app_config.config.cal_green_degree = degree;
            app_config.config.cal_green_x0 = x0;
            app_config.config.cal_green_x1 = x1;
            app_config.config.cal_green_x2 = x2;
            app_config.config.cal_green_x3 = x3;
            break;
        case str2int("black"):
            app_config.config.cal_black_degree = degree;
            app_config.config.cal_black_x0 = x0;
            app_config.config.cal_black_x1 = x1;
            app_config.config.cal_black_x2 = x2;
            app_config.config.cal_black_x3 = x3;
            break;
        case str2int("purple"):
            app_config.config.cal_purple_degree = degree;
            app_config.config.cal_purple_x0 = x0;
            app_config.config.cal_purple_x1 = x1;
            app_config.config.cal_purple_x2 = x2;
            app_config.config.cal_purple_x3 = x3;
            break;
        case str2int("orange"):
            app_config.config.cal_orange_degree = degree;
            app_config.config.cal_orange_x0 = x0;
            app_config.config.cal_orange_x1 = x1;
            app_config.config.cal_orange_x2 = x2;
            app_config.config.cal_orange_x3 = x3;
            break;
        case str2int("blue"):
            app_config.config.cal_blue_degree = degree;
            app_config.config.cal_blue_x0 = x0;
            app_config.config.cal_blue_x1 = x1;
            app_config.config.cal_blue_x2 = x2;
            app_config.config.cal_blue_x3 = x3;
            break;
        case str2int("yellow"):
            app_config.config.cal_yellow_degree = degree;
            app_config.config.cal_yellow_x0 = x0;
            app_config.config.cal_yellow_x1 = x1;
            app_config.config.cal_yellow_x2 = x2;
            app_config.config.cal_yellow_x3 = x3;
            break;
        case str2int("pink"):
            app_config.config.cal_pink_degree = degree;
            app_config.config.cal_pink_x0 = x0;
            app_config.config.cal_pink_x1 = x1;
            app_config.config.cal_pink_x2 = x2;
            app_config.config.cal_pink_x3 = x3;
            break;
         default:
             processCalibrationError(request);
     }

    redirectToCalibration(request);
}

String getContentType(String filename, AsyncWebServerRequest *request)
{
    if (request->hasArg("download"))
    {
        return "application/octet-stream";
    }
    else if (filename.endsWith(".htm"))
    {
        return "text/html";
    }
    else if (filename.endsWith(".html"))
    {
        return "text/html";
    }
    else if (filename.endsWith(".css"))
    {
        return "text/css";
    }
    else if (filename.endsWith(".js"))
    {
        return "application/javascript";
    }
    else if (filename.endsWith(".png"))
    {
        return "image/png";
    }
    else if (filename.endsWith(".gif"))
    {
        return "image/gif";
    }
    else if (filename.endsWith(".jpg"))
    {
        return "image/jpeg";
    }
    else if (filename.endsWith(".ico"))
    {
        return "image/x-icon";
    }
    else if (filename.endsWith(".xml"))
    {
        return "text/xml";
    }
    else if (filename.endsWith(".pdf"))
    {
        return "application/x-pdf";
    }
    else if (filename.endsWith(".zip"))
    {
        return "application/x-zip";
    }
    else if (filename.endsWith(".gz"))
    {
        return "application/x-gzip";
    }
    return "text/plain";
}

bool loadFromSpiffs(String path, AsyncWebServerRequest *request )
{
    DBG_OUTPUT_PORT.println("handleFileRead: " + path);
    String contentType = getContentType(path, request);
    //String pathWithGz = path + ".gz";
    //if (exists(pathWithGz) || exists(path))
    if (exists(path))
    {
        /*if (exists(pathWithGz))
        {
            path += ".gz";
        }*/
    
        request->send(SPIFFS,path,contentType);
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------------------

#ifndef DISABLE_OTA_UPDATES
void trigger_OTA(AsyncWebServerRequest *request) {
    loadFromSpiffs("/updating.htm", request);    // Send a message to the user to let them know what is going on
    app_config.config["update_spiffs"] = true;
    lcd.display_ota_update_screen();    // Trigger this here while everything else is waiting.
    delay(1000);                        // Wait 1 second to let everything send
    tilt_scanner.wait_until_scan_complete();    // Wait for scans to complete (we don't want any tasks running in the background)
    execOTA();                          // Trigger the OTA update
}
#endif

void trigger_wifi_reset(AsyncWebServerRequest *request) {
    loadFromSpiffs("/wifi_reset.htm",request);    // Send a message to the user to let them know what is going on                               // Wait 1 second to let everything send
    tilt_scanner.wait_until_scan_complete();    // Wait for scans to complete (we don't want any tasks running in the background)
    //disconnect_from_wifi_and_restart();         // Reset the wifi settings
}

void trigger_restart(AsyncWebServerRequest *request) {
    loadFromSpiffs("/restarting.htm", request);    // Send a message to the user to let them know what is going on
    http_server.restart_requested = true;
}

void http_json(AsyncWebServerRequest *request) {
    char tilt_data[1600];
    tilt_scanner.tilt_to_json_string(tilt_data,false);
    AsyncWebServerResponse *response = request->beginResponse(200,"application/json",tilt_data);
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

    char * config_js = (char *) malloc(sizeof(char) * 2500);
    app_config.dump_config(config_js);
    config_js[strlen(config_js)-1] = {'\0'};
    strcat(config_js,",\"all_valid\":");
    strcat(config_js,all_valid);
    strcat(config_js,"}");
    AsyncWebServerResponse *response = request->beginResponse(200,"application/json",config_js);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
    free(config_js);
    all_valid[0] = '1';
}

// About page Handlers
//

void this_version(AsyncWebServerRequest *request) {
    StaticJsonDocument<200> ver;

    ver["version"] = version();
    ver["branch"] = branch();
    ver["build"] = build();

    char output[200];
    serializeJson(ver,output);

    AsyncWebServerResponse *response = request->beginResponse(200,"application/json",output);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void uptime(AsyncWebServerRequest *request) {
    StaticJsonDocument<200> up;

    const int days = uptimeDays();
    const int hours = uptimeHours();
    const int minutes = uptimeMinutes();
    const int seconds = uptimeSeconds();
    const int millis = uptimeMillis();

    up["days"] = days;
    up["hours"] = hours;
    up["minutes"] = minutes;
    up["seconds"] = seconds;
    up["millis"] = millis;

    char output[200];
    serializeJson(up,output);

    AsyncWebServerResponse *response = request->beginResponse(200,"application/json",output);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void heap(AsyncWebServerRequest *request) {
    StaticJsonDocument<200> heap;

    uint32_t free = ESP.getFreeHeap();
    uint32_t max = ESP.getMaxAllocHeap();;
    uint8_t frag = 100 - (max * 100) / free;

    heap["free"] = free;
    heap["max"] = max;
    heap["frag"] = frag;

    char output[200];
    serializeJson(heap,output);

    AsyncWebServerResponse *response = request->beginResponse(200,"application/json",output);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void reset_reason(AsyncWebServerRequest *request) {
    StaticJsonDocument<200> rst;

    int reset = (int)esp_reset_reason();

    rst["reason"] = resetReason[reset];
    rst["description"] = resetDescription[reset];

    char output[200];
    serializeJson(rst,output);

    AsyncWebServerResponse *response = request->beginResponse(200,"application/json",output);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}


void httpServer::init(){
    server.serveStatic("/", SPIFFS, "/");
    server.rewrite("/", "/index.htm");
    server.rewrite("/about/","/about.htm");
    server.rewrite("/settings/","/settings.htm");
    server.rewrite("/calibration/","/calibration.htm");
    server.rewrite("/help/","/help.htm");

    server.on("/settings/update/",HTTP_POST,[](AsyncWebServerRequest *request){
        processConfig(request);
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
    // About Page Info Handlers
    server.on("/thisVersion/",HTTP_GET,[](AsyncWebServerRequest *request){
        this_version(request);
    });
    server.on("/uptime/",HTTP_GET,[](AsyncWebServerRequest *request){
        uptime(request);
    });
    server.on("/heap/",HTTP_GET,[](AsyncWebServerRequest *request){
        heap(request);
    });
    server.on("/resetreason/",HTTP_GET,[](AsyncWebServerRequest *request){
        reset_reason(request);
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

    server.onNotFound([](AsyncWebServerRequest *request) {
                // Look for other files on FILESYSTEM
        if (!loadFromSpiffs(request->url(),request))
        {
            AsyncWebServerResponse *response = request->beginResponse(404,"text/plain","File Not Found\n\n");
            request->send(response);
        }

    });

    server.begin();
}
