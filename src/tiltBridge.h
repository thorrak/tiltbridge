//
// Created by John Beeler on 5/12/18.
//

#ifndef TILTBRIDGE_TILTBRIDGE_H
#define TILTBRIDGE_TILTBRIDGE_H

//#include <nlohmann/json.hpp>
//
//// for convenience
//using json = nlohmann::json;

#include "tilt/tiltHydrometer.h"
#include "tilt/tiltScanner.h"
#include "bridge_lcd.h"
#include "jsonConfigHandler.h"


extern jsonConfigHandler app_config;

#define WIFI_SETUP_AP_NAME "TiltBridgeAP"
#define WIFI_SETUP_AP_PASS "tiltbridge"  // Must be 8-63 chars

#endif //TILTBRIDGE_TILTBRIDGE_H
