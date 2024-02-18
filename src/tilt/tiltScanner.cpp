//
// Created by John Beeler on 5/12/18.
//

#include <ArduinoLog.h>
#include <ArduinoJson.h>
#include <NimBLEDevice.h>

#include "tiltScanner.h"


// Create the scanner
BLEScan *pBLEScan;
tiltScanner tilt_scanner;

////////////////////////////
// BLE Scanner Callbacks/Code
////////////////////////////

void MyAdvertisedDeviceCallbacks::onResult(NimBLEAdvertisedDevice *advertisedDevice)
{
    if (advertisedDevice->getManufacturerData().length() >= 24)
    {
        if (advertisedDevice->getManufacturerData()[0] == 0x4c && advertisedDevice->getManufacturerData()[1] == 0x00 &&
            advertisedDevice->getManufacturerData()[2] == 0x02 && advertisedDevice->getManufacturerData()[3] == 0x15)
        {
#ifdef BLE_PRINT_ALL_DEVICES
            Log.verbose(F("Advertised iBeacon Device: %s \r\n"), advertisedDevice->toString().c_str());
#endif
            tilt_scanner.load_tilt_from_advert_hex(advertisedDevice->getManufacturerData(), advertisedDevice->getRSSI());
        }
    }
}

////////////////////////////
// tiltScanner Implementation
////////////////////////////

tiltScanner::tiltScanner()
{
    // Initialize by setting "m_scan_active" false
    for (uint8_t i = 0; i < TILT_COLORS; i++)
        m_tilt_devices[i] = new tiltHydrometer(i);

    // Also initialize the callbacks
    callbacks = new MyAdvertisedDeviceCallbacks();
}

void tiltScanner::init()
{
    shouldRun = true;
    NimBLEDevice::init("");
    pBLEScan = NimBLEDevice::getScan(); // Create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(callbacks);
    pBLEScan->setMaxResults(0);
    // Active scan actively queries devices for more info following detection.
    pBLEScan->setActiveScan(false);
    pBLEScan->setInterval(97); // Select prime numbers to reduce risk of frequency beat pattern with ibeacon advertisement interval
    pBLEScan->setWindow(37);   // Set to less or equal setInterval value. Leave reasonable gap to allow WiFi some time.

    tilt_scanner.scan();
}

void tiltScanner::deinit()
{
    wait_until_scan_complete();
    NimBLEDevice::deinit(); // Deinitialize the scanner & release memory
}

bool tiltScanner::scan()
{
    bool retval = false;
    if (shouldRun) {
        if (!pBLEScan->isScanning()) { // Check if scan already in progress
            //Try to start a new scan
            pBLEScan->clearResults();
            if (pBLEScan->start(BLE_SCAN_TIME, nullptr, true)) {
                retval = true; //Scan successfully started.
            } else {
                Log.verbose(F("Scan failed to start.\r\n"));
            }
        }
    }
    return retval;
}

bool tiltScanner::wait_until_scan_complete()
{
    if (!pBLEScan->isScanning())
        return false; // Return false if there wasn't a scan active when this was called

    while (pBLEScan->isScanning())
        delay(100); // Otherwise, keep sleeping 100ms at a time until the scan completes

    return true;
}

uint8_t tiltScanner::load_tilt_from_advert_hex(const std::string &advert_string_hex, const int8_t &current_rssi)
{
    uint8_t m_color;

    // Check that this is an iBeacon packet
    if (advert_string_hex[0] != 0x4c || advert_string_hex[1] != 0x00 || advert_string_hex[2] != 0x02 || advert_string_hex[3] != 0x15)
        return TILT_NONE;

    // The advertisement string is the "manufacturer data" part of the following:
    //Advertised Device: Name: Tilt, Address: 88:c2:55:ac:26:81, manufacturer data: 4c000215a495bb40c5b14b44b5121370f02d74de005004d9c5
    //4c000215a495bb40c5b14b44b5121370f02d74de005004d9c5
    //????????iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiittttggggXR
    //**********----------**********----------**********
    char hex_code[3] = {'\0'};
    char m_color_arr[33] = {'\0'};
    char temp_arr[5] = {'\0'};
    char grav_arr[5] = {'\0'};
    char tx_pwr_arr[3] = {'\0'};

    for (int i = 4; i < advert_string_hex.length(); i++)
    {
        sprintf(hex_code, "%.2x", advert_string_hex[i]);
        //Indices 4 - 19 each generate two characters of the color array
        if ((i > 3) && (i < 20))
        {
            strncat(m_color_arr, hex_code, 2);
        }
        //Indices 20-21 each generate two characters of the temperature array
        if (i == 20 || i == 21)
        {
            strncat(temp_arr, hex_code, 2);
        }
        //Indices 22-23 each generate two characters of the sp_gravity array
        if (i == 22 || i == 23)
        {
            strncat(grav_arr, hex_code, 2);
        }
        //Index 24 contains the tx_pwr (which is used by recent tilts to indicate battery age)
        if (i == 24)
        {
            strncat(tx_pwr_arr, hex_code, 2);
        }
    }

    m_color = tiltHydrometer::uuid_to_color_no(m_color_arr);
    if (m_color == TILT_NONE)
    { // We didn't match the uuid to a color (should only happen if new colors are released)
        return TILT_NONE;
    }

    uint16_t temp = std::strtoul(temp_arr, nullptr, 16);
    uint16_t gravity = std::strtoul(grav_arr, nullptr, 16);
    uint8_t tx_pwr = std::strtoul(tx_pwr_arr, nullptr, 16);

    m_tilt_devices[m_color]->set_values(temp, gravity, tx_pwr, current_rssi);

    return m_color;
}

tiltHydrometer *tiltScanner::tilt(uint8_t color)
{
    return m_tilt_devices[color];
}

void tiltScanner::tilt_to_json_string(char *all_tilt_json, bool use_raw_gravity)
{
    DynamicJsonDocument doc(TILT_ALL_DATA_SIZE);
    tilt_to_json(doc, use_raw_gravity);
    serializeJson(doc, all_tilt_json, TILT_ALL_DATA_SIZE);
}

void tiltScanner::tilt_to_json(DynamicJsonDocument &doc, bool use_raw_gravity)
{
    char tilt_data[TILT_DATA_SIZE];
    for(uint8_t i = 0; i < TILT_COLORS; i++)
    {
        if (m_tilt_devices[i]->is_loaded())
        {
            tilt_data[0] = {'\0'};
            m_tilt_devices[i]->to_json_string(tilt_data, use_raw_gravity);
            doc[tilt_color_names[i]] = serialized(tilt_data);
        }
    }
}
