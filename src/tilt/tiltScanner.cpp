//
// Created by John Beeler on 5/12/18.
//

#include "tiltHydrometer.h"
#include "tiltScanner.h"

#ifdef BLE_PRINT_ALL_DEVICES
#include <Arduino.h>
#endif

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>


// Create the scanner
BLEScan* pBLEScan;
tiltScanner tilt_scanner;




////////////////////////////
// BLE Scanner Callbacks/Code
////////////////////////////

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
//        uint8_t color = tilt_scanner.load_tilt_from_advert_hex(advertisedDevice.getManufacturerData());
        tilt_scanner.load_tilt_from_advert_hex(advertisedDevice.getManufacturerData());
#if defined(BLE_PRINT_ALL_DEVICES) && defined(DEBUG_PRINTS)
        Serial.printf("Advertised Device: %s \r\n", advertisedDevice.toString().c_str());
#endif
    }
};

static void ble_scan_complete(BLEScanResults scanResults) {
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
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);  // less or equal setInterval value
}


void tiltScanner::set_scan_active_flag(bool value) {
    m_scan_active = value;
}


bool tiltScanner::scan() {
    // Set a flag when we start asynchronously scanning to prevent multiple scans from being launched
    if(!m_scan_active) {
        pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
        pBLEScan->start(BLE_SCAN_TIME, ble_scan_complete);
        m_scan_active = true;
        return true;
    }
    return false;
}


bool tiltScanner::wait_until_scan_complete() {
    if(!m_scan_active)
        return false;  // Return false if there wasn't a scan active when this was called

    while(m_scan_active)
        FreeRTOS::sleep(100);  // Otherwise, keep sleeping 100ms at a time until the scan completes

    pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory

    pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory

    return true;
}


uint8_t tiltScanner::load_tilt_from_advert_hex(std::string advert_string_hex) {
    std::stringstream ss;
    std::string advert_string;
//    std::string m_part1;
//    std::string m_end_string;
    uint8_t m_color;

    // There is almost certainly a better way to do this
    // TODO - Rewrite this to not cast the grav/temp as a string & then recast it as an int
    char *hex_cstr = BLEUtils::buildHexData(nullptr, (uint8_t*)advert_string_hex.data(), advert_string_hex.length());
    ss.str(hex_cstr);
    free(hex_cstr);
    advert_string = ss.str();


    // We need the advert_string to be at least 50 characters long
    if(advert_string.length() < 50)
        return TILT_NONE;

    // The advertisement string is the "manufacturer data" part of the following:
    //Advertised Device: Name: Tilt, Address: 88:c2:55:ac:26:81, manufacturer data: 4c000215a495bb40c5b14b44b5121370f02d74de005004d9c5
    //4c000215a495bb40c5b14b44b5121370f02d74de005004d9c5
    //????????iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiittttggggXR
    //**********----------**********----------**********

    m_color = tiltHydrometer::uuid_to_color_no(advert_string.substr(8,32));

    if(m_color == TILT_NONE) // We didn't match the uuid to a color (should only happen if new colors are released)
        return TILT_NONE;

    uint32_t temp = std::stoul(advert_string.substr(40,4), nullptr, 16);
    uint32_t gravity = std::stoul(advert_string.substr(44,4), nullptr, 16);
//    m_end_string = advert_string.substr(48,2);  // first byte is txpower, second is RSSI

    m_tilt_devices[m_color]->set_values(temp, gravity);

    return m_color;
}


tiltHydrometer* tiltScanner::tilt(uint8_t color) {
    return m_tilt_devices[color];
}

json tiltScanner::tilt_to_json() {
    json j;
    for(uint8_t i = 0;i<TILT_COLORS;i++) {
        if(m_tilt_devices[i]->is_loaded()) {
            j[m_tilt_devices[i]->color_name()] = m_tilt_devices[i]->to_json();
        }
    }
    return j;
}