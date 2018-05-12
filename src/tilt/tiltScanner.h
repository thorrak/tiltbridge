//
// Created by John Beeler on 5/12/18.
//

#ifndef TILTBRIDGE_TILTSCANNER_H
#define TILTBRIDGE_TILTSCANNER_H

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

extern BLEScan* pBLEScan;


class tiltScanner {
public:
    tiltScanner();
    void init();
    void scan();


};

extern tiltScanner tilt_scanner;

#endif //TILTBRIDGE_TILTSCANNER_H
