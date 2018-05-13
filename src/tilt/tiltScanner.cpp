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




////////////////////////////
// BLE Scanner Callbacks/Code
////////////////////////////

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
//        tiltHydrometer tilt;
//        Serial.printf("Advertised Device: %s \r\n", advertisedDevice.toString().c_str());
        if(advertisedDevice.getName() == "Tilt") {
            uint8_t color = tilt_scanner.load_tilt_from_advert_hex(advertisedDevice.getManufacturerData());
            if(color == TILT_NONE)
                Serial.printf("Failed to load tilt - ");
            else
                Serial.printf("Successfully loaded tilt - ");

            tiltHydrometer tilt = *tilt_scanner.tilt(color);

            Serial.printf("Color: %s, Temp: %s, Grav: %s \r\n",
                          tilt.color_name().c_str(), tilt.m_temp_string.c_str(), tilt.m_gravity_string.c_str());
            Serial.printf("Temp F: %d, Grav: %d \r\n",
                          tilt.temp, tilt.gravity);
        }
    }
};

static void ble_scan_complete(BLEScanResults scanResults) {
//    printf("Scan complete!\n");
//    printf("We found %d devices\n", scanResults.getCount());
//    scanResults.dump();
    tilt_scanner.set_scan_active_flag(false);
}



////////////////////////////
// tiltScanner Implementation
////////////////////////////


tiltScanner::tiltScanner() {
    // Initialize by setting "m_scan_active" false
    m_scan_active = false;
    for(uint8_t i = 0;i<TILT_COLORS;i++)
        m_tilt_devices[i] = new tiltHydrometer(i);
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


uint8_t tiltScanner::load_tilt_from_advert_hex(std::string advert_string_hex) {
    std::stringstream ss;
    std::string advert_string;

    // There is almost certainly a better way to do this
    // TODO - Rewrite this to not cast the grav/temp as a string & then recast it as an int
    char *hex_cstr = BLEUtils::buildHexData(nullptr, (uint8_t*)advert_string_hex.data(), advert_string_hex.length());
    ss.str(hex_cstr);
    free(hex_cstr);
    advert_string = ss.str();


//    std::string m_part1;
    std::string device_uuid;
    std::string temp_string;
    std::string gravity_string;
//    std::string m_end_string;
    uint8_t m_color;

    // We need the advert_string to be at least 50 characters long
    if(advert_string.length() < 50)
        return TILT_NONE;

    // The advertisement string is the "manufacturer data" part of the following:
    //Advertised Device: Name: Tilt, Address: 88:c2:55:ac:26:81, manufacturer data: 4c000215a495bb40c5b14b44b5121370f02d74de005004d9c5
    //4c000215a495bb40c5b14b44b5121370f02d74de005004d9c5
    //????????iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiittttgggg??
    //**********----------**********----------**********

//    m_part1 = advert_string.substr(0,8);
    device_uuid = advert_string.substr(8,32);
    m_color = tiltHydrometer::uuid_to_color_no(device_uuid);

    if(!m_color) // We didn't match the uuid to a color
        return TILT_NONE;

    temp_string = advert_string.substr(40,4);
    gravity_string = advert_string.substr(44,4);
//    m_end_string = advert_string.substr(48,2);  // first byte is txpower, second is RSSI
    uint32_t temp = std::stoul(temp_string, nullptr, 16);
    uint32_t gravity = std::stoul(gravity_string, nullptr, 16);

    m_tilt_devices[m_color]->set_string_values(temp_string, gravity_string);
    m_tilt_devices[m_color]->set_values(temp, gravity);

    return m_color;

}


tiltHydrometer* tiltScanner::tilt(uint8_t color) {
    return m_tilt_devices[color];
}