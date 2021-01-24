//
// Created by John Beeler on 6/4/18.
//

#ifndef TILTBRIDGE_WIFI_SETUP_H
#define TILTBRIDGE_WIFI_SETUP_H

#define WIFI_SETUP_AP_NAME "TiltBridgeAP"
#define WIFI_SETUP_AP_PASS "tiltbridge" // Must be 8-63 chars

#if defined (LCD_TFT_ESPI) || defined (LCD_SSD1306)
#define BOARD_RESET_BUTTON_GPIO 35        // Using the "reset" button
#define WIFI_RESET_BUTTON_GPIO 0          // Using the "boot" button
#endif
#define WIFI_RESET_DOUBLE_PRESS_TIME 3000 // How long (in ms) the user has to press the wifi reset button a second time

#include "bridge_lcd.h"
#include "serialhandler.h"
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
void initBoardResetButton();
void disconnect_from_wifi_and_restart();
void handle_wifi_reset_presses();
void reconnectIfDisconnected();

extern unsigned long boardResetTime;

#endif //TILTBRIDGE_WIFI_SETUP_H
