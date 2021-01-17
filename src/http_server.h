//
// Created by John Beeler on 2/17/19.
//

#ifndef TILTBRIDGE_HTTP_SERVER_H
#define TILTBRIDGE_HTTP_SERVER_H

#include "uptime.h"
#include "version.h"
#include "tiltBridge.h"
#include "wifi_setup.h"
#include "tilt/tiltScanner.h"
#include "OTAUpdate.h"
#include "sendData.h"
#include "jsonconfig.h"

#include <LCBUrl.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#ifdef FSEDIT
#include <SPIFFSEditor.h>
#endif

#if FILESYSTEM == SPIFFS
#include <SPIFFS.h>
#endif

#define BREWFATHER_MIN_KEY_LENGTH 5
#define BREWERS_FRIEND_MIN_KEY_LENGTH 12
#define BREWSTATUS_MIN_KEY_LENGTH 12

class httpServer
{
public:
    void init();
    //void handleClient();
    bool lcd_reinit_rqd = false;
    bool restart_requested = false;
    bool name_reset_requested = false;
    bool wifi_reset_requested = false;
    bool factoryreset_requested = false;
    bool mqtt_init_rqd = false;
};

extern httpServer http_server;

#endif //TILTBRIDGE_HTTP_SERVER_H
