#ifndef TILTBRIDGE_WIFI_SETUP_H
#define TILTBRIDGE_WIFI_SETUP_H

#define WIFI_SETUP_AP_NAME "TiltBridgeAP"
#define WIFI_SETUP_AP_PASS "tiltbridge" // Must be 8-63 chars

//#include "bridge_lcd.h"
//#include "serialhandler.h"
//#include "jsonconfig.h"
//#include <Arduino.h>

//#include <ESPmDNS.h>
//#include <WiFiClient.h>
//#include <LCBUrl.h>
//#include "http_server.h" // Make sure this include is after AsyncWiFiManager

#define WEB_SERVER_PORT 80


void initWiFi();
void mdnsReset();

void disconnectWiFi();
void reconnectWiFi();

#endif //TILTBRIDGE_WIFI_SETUP_H
