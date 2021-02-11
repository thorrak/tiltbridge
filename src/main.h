#ifndef _MAIN_H
#define _MAIN_H

#include "watchButtons.h"
#include "http_server.h"
#include "sendData.h"
#include "tilt/tiltScanner.h"
#include "bridge_lcd.h"
#include "wifi_setup.h"
#include "serialhandler.h"
#include "jsonconfig.h"
#include <Arduino.h>
#include <Ticker.h>
#include <esp_log.h>

#ifdef LOG_LOCAL_LEVEL
#include <esp_log.h>
#endif

void printMem();
void setup();
void loop();

extern bool doWiFiReset;
extern bool doBoardReset;

#endif
