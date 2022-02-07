#ifndef _PARSE_TARGET_H
#define _PARSE_TARGET_H

#include "version.h"
#include "getGuid.h"
#include "jsonconfig.h"
#include "tilt/tiltScanner.h"
#include "tilt/tiltHydrometer.h"
#include <ArduinoLog.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Parse.h>
#include <Arduino.h>

void doParsePoll();
void doParseSetup();
void addTiltToParse();

extern const char* tilt_color_names[];

#endif // _PARSE_TARGET_H
