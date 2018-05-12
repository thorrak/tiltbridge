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


}


void tiltScanner::init() {
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan(); //create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
}


void tiltScanner::scan() {
    BLEScanResults foundDevices = pBLEScan->start(scanTime);
}

