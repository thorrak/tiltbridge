//
// Created by John Beeler on 5/12/18.
// Modified by Tim Pletcher on 31-Oct-2020.
//

#ifndef TILTBRIDGE_TILTSCANNER_H
#define TILTBRIDGE_TILTSCANNER_H

#include "tiltBridge.h"
#include "tiltHydrometer.h"
#include "serialhandler.h"
#include "tiltHydrometer.h"
#include <NimBLEAdvertisedDevice.h>
#include <ArduinoJson.h>
#include <NimBLEDevice.h>
#include <NimBLEUtils.h>
#include <NimBLEScan.h>
#include <NimBLEAdvertisedDevice.h>

#define BLE_SCAN_TIME 3 // Seconds to scan

class MyAdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks
{
    void onResult(NimBLEAdvertisedDevice *advertisedDevice) override;
};

class tiltScanner
{
public:
    tiltScanner();
    void init();
    void deinit();
    bool scan();

    bool wait_until_scan_complete();
    void set_scan_active_flag(bool value);
    uint8_t load_tilt_from_advert_hex(const std::string &advert_string_hex);
    void tilt_to_json_string(char *json_string, bool use_raw_gravity);

    tiltHydrometer *tilt(uint8_t color);

private:
    bool m_scan_active;
    tiltHydrometer *m_tilt_devices[TILT_COLORS]{};
    MyAdvertisedDeviceCallbacks *callbacks;
};

extern tiltScanner tilt_scanner;

#endif //TILTBRIDGE_TILTSCANNER_H