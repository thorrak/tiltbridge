//
// Created by John Beeler on 6/4/18.
//

#ifndef TILTBRIDGE_WIFI_SETUP_H
#define TILTBRIDGE_WIFI_SETUP_H

#include "serialhandler.h"
#include "tiltBridge.h"
#include "jsonconfig.h"
#include <Arduino.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include <LCBUrl.h>
#include "http_server.h" // Make sure this include is after AsyncWiFiManager

void init_wifi();
void mdnsreset();
void initWiFiResetButton();
void disconnect_from_wifi_and_restart();
void handle_wifi_reset_presses();
void reconnectIfDisconnected();

#endif //TILTBRIDGE_WIFI_SETUP_H
