//
// Created by John Beeler on 5/12/18.
//

#ifndef TILTBRIDGE_TILTSCANNER_H
#define TILTBRIDGE_TILTSCANNER_H

#include <NimBLEAdvertisedDevice.h>
#include <ArduinoJson.h>

#include "tiltHydrometer.h"


// Due to a crash that I've seen when restarting scans, I'm going to switch to using a long-lived
// scan rather than restarting every few seconds. 
#define BLE_SCAN_TIME 12 * 60 * 60 * 1000  // Milliseconds to scan

class ScanCallbacks: public NimBLEScanCallbacks
{
    void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override;
};

class tiltScanner
{
public:
    tiltScanner();
    void init();
    void deinit();
    bool scan();

    bool wait_until_scan_complete();
    uint8_t load_tilt_from_advert_hex(const std::string &advert_string_hex, const int8_t &current_rssi);
    void tilt_to_json(JsonDocument &doc, bool use_raw_gravity);

    tiltHydrometer *tilt(uint8_t color);

private:
    tiltHydrometer *m_tilt_devices[TILT_COLORS]{};
    ScanCallbacks *callbacks;
    bool shouldRun;
};

extern tiltScanner tilt_scanner;

#endif //TILTBRIDGE_TILTSCANNER_H
