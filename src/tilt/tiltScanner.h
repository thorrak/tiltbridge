//
// Created by John Beeler on 5/12/18.
//

#ifndef TILTBRIDGE_TILTSCANNER_H
#define TILTBRIDGE_TILTSCANNER_H

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#define BLE_SCAN_TIME 15


extern BLEScan* pBLEScan;


class tiltScanner {
public:
    tiltScanner();
    void init();
    void scan();

    bool wait_until_scan_complete();
    void set_scan_active_flag(bool value);

private:
    bool m_scan_active;

};

extern tiltScanner tilt_scanner;

#endif //TILTBRIDGE_TILTSCANNER_H
