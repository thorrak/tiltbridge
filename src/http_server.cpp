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

// This is to simplify the redirects in processConfig
void redirectToConfig() {
    server.sendHeader("Location", "/settings/");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
}

void processConfig() {

    Serial.println("Entered processConfig");
    if (server.hasArg("fermentrackURL")) {
        Serial.println("Has fermentrackURL");
        app_config.config["fermentrackURL"] = server.arg("fermentrackURL").c_str();
        Serial.println("Updated fermentrackURL");
        Serial.println(app_config.config.dump().c_str());
        redirectToConfig();
    }

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
    // TODO - Determine if I want to shift this to be inline
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", app_config.config.dump().c_str());
}

//no need authentification
void handleNotFound() {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
}


void httpServer::init(){
    Serial.println("");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

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