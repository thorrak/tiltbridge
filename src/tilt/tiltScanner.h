//
// Created by John Beeler on 5/12/18.
//

#ifndef TILTBRIDGE_TILTSCANNER_H
#define TILTBRIDGE_TILTSCANNER_H

#include "tiltHydrometer.h"
#include "serialhandler.h"
#include "tiltHydrometer.h"

#include <Ticker.h>
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
    uint8_t load_tilt_from_advert_hex(const std::string &advert_string_hex, const int8_t &current_rssi);
    void tilt_to_json_string(char *json_string, bool use_raw_gravity);

    tiltHydrometer *tilt(uint8_t color);

private:
    tiltHydrometer *m_tilt_devices[TILT_COLORS]{};
    MyAdvertisedDeviceCallbacks *callbacks;
};

void pingScanner();

extern tiltScanner tilt_scanner;

#endif //TILTBRIDGE_TILTSCANNER_H
