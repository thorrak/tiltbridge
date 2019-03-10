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

httpServer http_server;

WebServer server(80);

//Check if header is present and correct
bool is_authentified() {
    Serial.println("Enter is_authentified");
    if (server.hasHeader("Cookie")) {
        Serial.print("Found cookie: ");
        String cookie = server.header("Cookie");
        Serial.println(cookie);
        if (cookie.indexOf("ESPSESSIONID=1") != -1) {
            Serial.println("Authentification Successful");
            return true;
        }
    } else if (server.hasArg("PASSWORD")) {

    }



    Serial.println("Authentification Failed");
    return false;
}

//login page, also called for disconnect
void handleLogin() {
    String msg;
    if (server.hasHeader("Cookie")) {
        Serial.print("Found cookie: ");
        String cookie = server.header("Cookie");
        Serial.println(cookie);
    }
    if (server.hasArg("DISCONNECT")) {
        Serial.println("Disconnection");
        server.sendHeader("Location", "/login");
        server.sendHeader("Cache-Control", "no-cache");
        server.sendHeader("Set-Cookie", "ESPSESSIONID=0");
        server.send(301);
        return;
    }
    if (server.hasArg("USERNAME") && server.hasArg("PASSWORD")) {
        if (server.arg("USERNAME") == "admin" &&  server.arg("PASSWORD") == "admin") {
            server.sendHeader("Location", "/");
            server.sendHeader("Cache-Control", "no-cache");
            server.sendHeader("Set-Cookie", "ESPSESSIONID=1");
            server.send(301);
            Serial.println("Log in Successful");
            return;
        }
        msg = "Wrong username/password! try again.";
        Serial.println("Log in Failed");
    }
    String content = "<html><body><form action='/login' method='POST'>To log in, please use : admin/admin<br>";
    content += "User:<input type='text' name='USERNAME' placeholder='user name'><br>";
    content += "Password:<input type='password' name='PASSWORD' placeholder='password'><br>";
    content += "<input type='submit' name='SUBMIT' value='Submit'></form>" + msg + "<br>";
    content += "You also can go <a href='/inline'>here</a></body></html>";
    server.send(200, "text/html", content);
}


////root page can be accessed only if authentification is ok
//void http_root() {
//    nlohmann::json json_obj;
//
//    Serial.println("Enter http_root");
//
//    json_obj.clear();
//    json_obj["tilts"] = tilt_scanner.tilt_to_json();
//
//    if(strlen(json_obj.dump().c_str()) > 5) {
//        Serial.print("http_root data: ");
//        Serial.println(json_obj.dump().c_str());
//    } else {
//        Serial.print("No data to send.");
//    }
//
//    String header;
//    String content = "<html><body><H2>hello, you successfully connected to esp8266!</H2><br>";
//    content += "<p>The tilt data we would send is: ";
//    content += json_obj.dump().c_str();
//    content += "</p>";
//
//    if (server.hasHeader("User-Agent")) {
//        content += "the user agent used is : " + server.header("User-Agent") + "<br><br>";
//    }
//    content += "You can access this page until you <a href=\"/login?DISCONNECT=YES\">disconnect</a></body></html>";
//    server.send(200, "text/html", content);
//}

inline bool isInteger(const char*  s)
{
    // TODO - Fix this
    if(strlen(s) <= 0 || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) return false;

    char * p;
    strtol(s, &p, 10);

    return (*p == 0);
}

// This is to simplify the redirects in processConfig
void redirectToConfig() {
    // TODO - Disable this
    Serial.println(app_config.config.dump().c_str());

    server.sendHeader("Location", "/settings/");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
}

void processConfigError() {
    Serial.println("processConfigError!");
    redirectToConfig();
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

    Serial.println("Entered processConfig");

    // Generic TiltBridge Settings
    if (server.hasArg("mdnsID")) {
        if (server.arg("mdnsID").length() > 30)
            return processConfigError();
        else if (server.arg("mdnsID").length() < 3)
            return processConfigError();
        else
            app_config.config["mdnsID"] = server.arg("mdnsID").c_str();
    }


    // Fermentrack Settings
    if (server.hasArg("fermentrackURL")) {
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

    if (server.hasArg("fermentrackToken")) {
        if (server.arg("fermentrackToken").length() > 255)
            return processConfigError();
        else if (server.arg("fermentrackToken").length() < 1)
            app_config.config["fermentrackToken"] = "";
        else
            app_config.config["fermentrackToken"] = server.arg("fermentrackToken").c_str();
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
        else if (server.arg("brewersFriendKey").length() < 7)
            app_config.config["brewersFriendKey"] = "";
        else
            app_config.config["brewersFriendKey"] = server.arg("brewersFriendKey").c_str();
    }

    // If we made it this far, one or more settings were updated. Save.
    app_config.save();
    redirectToConfig();
}




//This function is overkill for how we're handling things, but
bool loadFromSpiffs(String path)
{
    String dataType = "text/plain";
//    if (path.endsWith("/")) path += "index.htm"; //this is where index.htm is created

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
    if (!dataFile)  //unsuccesful open
    {
        Serial.print("Don't know this command and it's not a file in SPIFFS : ");
        Serial.println(path);
        return false;
    }
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

void http_json() {
    // I probably don't want this inline so that I can add the Allow-Origin header (in case anyone wants to build
    // scripts that pull this data)
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", tilt_scanner.tilt_to_json().dump().c_str());
}

// settings_json is intended to be used to build the "Change Settings" page
void settings_json() {
    // TODO - Determine if I want to remove allow-origin
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", app_config.config.dump().c_str());
}


void handleNotFound() {
    String message = "File Not Found\n\n";
//    message += "URI: ";
//    message += server.uri();
//    message += "\nMethod: ";
//    message += (server.method() == HTTP_GET) ? "GET" : "POST";
//    message += "\nArguments: ";
//    message += server.args();
//    message += "\n";
//    for (uint8_t i = 0; i < server.args(); i++) {
//        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
//    }
    server.send(404, "text/plain", message);
}


void httpServer::init(){
    server.on("/", root_from_spiffs);
    server.on("/settings/", settings_from_spiffs);
    server.on("/settings/update/", processConfig);

    server.on("/json/", http_json);
    server.on("/settings/json/", settings_json);
//    server.on("/login", handleLogin);
//    server.on("/inline", []() {
//        server.send(200, "text/plain", "this works without need of authentification");
//    });

//    server.onNotFound(handleNotFound);
    server.onNotFound(handleNotFound);
    //here the list of headers to be recorded
//    const char * headerkeys[] = {"User-Agent", "Cookie"} ;
//    size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
    //ask server to track these headers
//    server.collectHeaders(headerkeys, headerkeyssize);
    server.begin();
    Serial.println("HTTP server started");
}

void httpServer::handleClient(){
    server.handleClient();
}