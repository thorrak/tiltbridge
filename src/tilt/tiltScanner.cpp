//
// Created by John Beeler on 5/12/18.
//

#include <nlohmann/json.hpp>
#include <Arduino.h>

#include "tiltScanner.h"
#include "tiltHydrometer.h"


// Create the scanner
BLEScan* pBLEScan;
tiltScanner tilt_scanner;

uint32_t scanTime = 30; //In seconds



class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        tiltHydrometer tilt;
//        Serial.printf("Advertised Device: %s \r\n", advertisedDevice.toString().c_str());
        if(advertisedDevice.getName() == "Tilt") {
            if(!tilt.load_from_advert_hex(advertisedDevice.getManufacturerData()))
                Serial.printf("Failed to load tilt - ");
            else
                Serial.printf("Successfully loaded tilt - ");

            Serial.printf("Color: %s, Temp: %s, Grav: %s \r\n",
                          tilt.color_name().c_str(), tilt.m_temp_string.c_str(), tilt.m_gravity_string.c_str());
        }
    }
};


tiltScanner::tiltScanner() {
    // Initialize by setting "m_scan_active" false
    m_scan_active = false;
}


void tiltScanner::init() {
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan(); //create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), false);
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
}

void tiltScanner::set_scan_active_flag(bool value) {
    m_scan_active = value;
}


static void ble_scan_complete(BLEScanResults scanResults) {
//    printf("Scan complete!\n");
//    printf("We found %d devices\n", scanResults.getCount());
//    scanResults.dump();
    tilt_scanner.set_scan_active_flag(false);
}

void tiltScanner::scan() {
    // Set a flag when we start asynchronously scanning to prevent
    if(!m_scan_active) {
        pBLEScan->start(BLE_SCAN_TIME, ble_scan_complete);
        m_scan_active = true;
    }
}


bool tiltScanner::wait_until_scan_complete() {
    if(!m_scan_active)
        return false;  // Return false if there wasn't a scan active when this was called

    while(m_scan_active)
        FreeRTOS::sleep(100);  // Otherwise, sleep for 100ms until the scan completes

    return true;
}
