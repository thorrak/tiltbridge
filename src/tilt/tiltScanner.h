//
// Created by John Beeler on 5/12/18.
//

#ifndef TILTBRIDGE_TILTSCANNER_H
#define TILTBRIDGE_TILTSCANNER_H

#include "tiltHydrometer.h"
#include <nlohmann/json.hpp>
#include <NimBLEAdvertisedDevice.h>



#define BLE_SCAN_TIME       3  // Seconds to scan


// for convenience
using json = nlohmann::json;


class MyAdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) override;
};


class tiltScanner {
public:
    tiltScanner();
    void init();
    void deinit();
    bool scan();

    bool wait_until_scan_complete();
    void set_scan_active_flag(bool value);
    uint8_t load_tilt_from_advert_hex(const std::string& advert_string_hex);
    nlohmann::json tilt_to_json();


    tiltHydrometer* tilt(uint8_t color);

private:
    bool m_scan_active;
    tiltHydrometer* m_tilt_devices[TILT_COLORS]{};
    MyAdvertisedDeviceCallbacks *callbacks;
};

extern tiltScanner tilt_scanner;

#endif //TILTBRIDGE_TILTSCANNER_H