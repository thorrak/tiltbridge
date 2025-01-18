//
// Created by John Beeler on 5/12/18.
//

#ifndef TILTBRIDGE_TILTSCANNER_H
#define TILTBRIDGE_TILTSCANNER_H

#include <NimBLEAdvertisedDevice.h>
#include <ArduinoJson.h>
#include <list>

#include "tiltHydrometer.h"


#define BLE_SCAN_TIME 3 * 1000  // Milliseconds to scan

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
    uint8_t load_tilt_from_advert_hex(const NimBLEAdvertisedDevice* advertisedDevice);
    void tilt_to_json(JsonDocument &doc, bool use_raw_gravity);

    std::size_t tilt_count();
    std::list<tiltHydrometer> m_tilt_devices;

    tiltHydrometer* get_tilt(const NimBLEAddress devAddress);
    tiltHydrometer* get_or_create_tilt(const NimBLEAddress devAddress);

private:
    ScanCallbacks *callbacks;
    bool shouldRun;
};

extern tiltScanner tilt_scanner;

#endif //TILTBRIDGE_TILTSCANNER_H
