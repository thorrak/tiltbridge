//
// Created by John Beeler on 5/12/18.
//

#ifndef TILTBRIDGE_TILTSCANNER_H
#define TILTBRIDGE_TILTSCANNER_H

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include "tiltHydrometer.h"

#define BLE_SCAN_TIME       15  // Seconds to scan
#define TILT_NONE           255 // Alternative to a tilt color (should arguably be in tiltHydrometer.h)



extern BLEScan* pBLEScan;


class tiltScanner {
public:
    tiltScanner();
    void init();
    void scan();

    bool wait_until_scan_complete();
    void set_scan_active_flag(bool value);
    uint8_t load_tilt_from_advert_hex(std::string advert_string_hex);

    tiltHydrometer* tilt(uint8_t color);

private:
    bool m_scan_active;
    tiltHydrometer* m_tilt_devices[];
};

extern tiltScanner tilt_scanner;

#endif //TILTBRIDGE_TILTSCANNER_H
