//
// Created by John Beeler on 5/12/18.
//

#ifndef TILTBRIDGE_TILTSCANNER_H
#define TILTBRIDGE_TILTSCANNER_H

#include "../../.piolibdeps/ESP32 BLE Arduino_ID1841/src/BLEDevice.h"
#include "../../.piolibdeps/ESP32 BLE Arduino_ID1841/src/BLEUtils.h"
#include "../../.piolibdeps/ESP32 BLE Arduino_ID1841/src/BLEScan.h"
#include "../../.piolibdeps/ESP32 BLE Arduino_ID1841/src/BLEAdvertisedDevice.h"

extern BLEScan* pBLEScan;


class tiltScanner {
public:
    tiltScanner();
    void init();
    void scan();


};

extern tiltScanner tilt_scanner;

#endif //TILTBRIDGE_TILTSCANNER_H
