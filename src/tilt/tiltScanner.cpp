//
// Created by John Beeler on 5/12/18.
//

#include "tiltBridge.h"
#include "tiltHydrometer.h"
#include "tiltScanner.h"

#ifdef BLE_PRINT_ALL_DEVICES
#include <Arduino.h>
#endif

#include <NimBLEDevice.h>
#include <NimBLEUtils.h>
#include <NimBLEScan.h>
#include <NimBLEAdvertisedDevice.h>


// Create the scanner
BLEScan* pBLEScan;
tiltScanner tilt_scanner;




////////////////////////////
// BLE Scanner Callbacks/Code
////////////////////////////

void MyAdvertisedDeviceCallbacks::onResult(NimBLEAdvertisedDevice* advertisedDevice) {
//        uint8_t color = tilt_scanner.load_tilt_from_advert_hex(advertisedDevice.getManufacturerData());

    if(advertisedDevice->getManufacturerData().length() > 4) {
        if(advertisedDevice->getManufacturerData()[0] == 0x4c && advertisedDevice->getManufacturerData()[1] == 0x00 &&
           advertisedDevice->getManufacturerData()[2] == 0x02 && advertisedDevice->getManufacturerData()[3] == 0x15) {
#if defined(BLE_PRINT_ALL_DEVICES) && defined(DEBUG_PRINTS)
            Serial.printf("Advertised iBeacon Device: %s ", advertisedDevice->toString().c_str());
            Serial.println();
#endif
            tilt_scanner.load_tilt_from_advert_hex(advertisedDevice->getManufacturerData());
        }
    }
}

static void ble_scan_complete(NimBLEScanResults scanResults) {
    // Todo - Decide if it makes sense to process scan results here rather than on a callback
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

    // Also initialize the callbacks
    callbacks = new MyAdvertisedDeviceCallbacks();
}


void tiltScanner::init() {
    NimBLEDevice::init("");
    pBLEScan = NimBLEDevice::getScan(); //create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(callbacks);
    // CHANGE IN NIMBLE - Active Scan must be set false to read new (Gen 2/3) Tilts
    // See https://github.com/h2zero/NimBLE-Arduino/issues/22
    pBLEScan->setActiveScan(false); //active scan uses more power, but get results faster
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);  // less or equal setInterval value
}

void tiltScanner::deinit() {
    //pBLEScan->stop();
    wait_until_scan_complete();
    // NimBLE fails to reinitialize after a call to deinit() (but thankfully it's light enough weight that we don't
    // have to call deinit to use https any longer)
    // https://github.com/h2zero/NimBLE-Arduino/issues/23
    //NimBLEDevice::deinit();  // Deinitialize the scanner & release memory
}


void tiltScanner::set_scan_active_flag(bool value) {
    m_scan_active = value;
}


bool tiltScanner::scan() {
    // Set a flag when we start asynchronously scanning to prevent multiple scans from being launched
    if(!m_scan_active) {
        pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory

        if(!pBLEScan->start(BLE_SCAN_TIME, ble_scan_complete, true)) {
            // We failed to start a scan. There is a race condition where ble_scan_complete gets triggered prior to
            // the semaphores being released - if we happen to catch things just right, we can end up in an inconsistent
            // state.
#ifdef DEBUG_PRINTS
            Serial.println("Scan already in progress - explicitly stopping.");
#endif
            pBLEScan->stop();
            delay(100);
            return false;
        } else {
            // We successfully started a scan
            m_scan_active = true;
            return true;
        }
    }
    return false;
}


bool tiltScanner::wait_until_scan_complete() {
    if(!m_scan_active)
        return false;  // Return false if there wasn't a scan active when this was called

    while(m_scan_active)
        delay(100);  // Otherwise, keep sleeping 100ms at a time until the scan completes

    //pBLEScan->stop();
    pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory

    return true;
}


uint8_t tiltScanner::load_tilt_from_advert_hex(const std::string& advert_string_hex) {
    std::stringstream ss;
    std::string advert_string;
//    std::string m_part1;
//    std::string m_end_string;
    uint8_t m_color;

    // Check that this is an iBeacon packet
    if(advert_string_hex[0] != 0x4c || advert_string_hex[1] != 0x00 || advert_string_hex[2] != 0x02 || advert_string_hex[3] != 0x15)
        return TILT_NONE;

    // There is almost certainly a better way to do this
    char *hex_cstr = NimBLEUtils::buildHexData(nullptr, (uint8_t*)advert_string_hex.data(), advert_string_hex.length());
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

    // TODO - Rewrite this to not cast the grav/temp as a string & then recast it as an int
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