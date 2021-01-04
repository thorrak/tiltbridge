//
// Created by John Beeler on 5/12/18.
//

#ifndef TILTBRIDGE_TILTBRIDGE_H
#define TILTBRIDGE_TILTBRIDGE_H

#include "tilt/tiltHydrometer.h"
#include "tilt/tiltScanner.h"
#include "bridge_lcd.h"
#include "jsonConfigHandler.h"

#define WIFI_SETUP_AP_NAME "TiltBridgeAP"
#define WIFI_SETUP_AP_PASS "tiltbridge" // Must be 8-63 chars

#define WIFI_RESET_BUTTON_GPIO 0          // Using the "boot" button
#define WIFI_RESET_DOUBLE_PRESS_TIME 3000 // How long (in ms) the user has to press the wifi reset button a second time

// Enable this for testing
//#define BLE_PRINT_ALL_DEVICES 1

// This is for a "heartbeat" checkin to fermentrack.com. Unless you are me (thorrak) don't enable this, please.
//#define ENABLE_TEST_CHECKINS 1

#endif //TILTBRIDGE_TILTBRIDGE_H
