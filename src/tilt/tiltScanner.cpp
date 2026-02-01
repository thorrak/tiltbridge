//
// Created by John Beeler on 5/12/18.
//

#include <thorlog.h>
#include <ArduinoJson.h>
#include <NimBLEDevice.h>

#include "tiltScanner.h"


// Create the scanner
BLEScan *pBLEScan;
tiltScanner tilt_scanner;

////////////////////////////
// BLE Scanner Callbacks/Code
////////////////////////////

void ScanCallbacks::onResult(const NimBLEAdvertisedDevice* advertisedDevice)
{
    if (advertisedDevice->getManufacturerData().length() >= 24)
    {
        if (advertisedDevice->getManufacturerData()[0] == 0x4c && advertisedDevice->getManufacturerData()[1] == 0x00 &&
            advertisedDevice->getManufacturerData()[2] == 0x02 && advertisedDevice->getManufacturerData()[3] == 0x15)
        {
#ifdef BLE_PRINT_ALL_DEVICES
            Log.verbose("Advertised iBeacon Device: %s \r\n", advertisedDevice->toString().c_str());
#endif
            tilt_scanner.load_tilt_from_advert_hex(advertisedDevice);
        }
    }
}

////////////////////////////
// tiltScanner Implementation
////////////////////////////

tiltScanner::tiltScanner()
{
    // Initialize the callbacks
    callbacks = new ScanCallbacks();
}

void tiltScanner::init()
{
    shouldRun = true;
    NimBLEDevice::init("");
    pBLEScan = NimBLEDevice::getScan(); // Create new scan
    pBLEScan->setScanCallbacks(callbacks);
    pBLEScan->setMaxResults(0);
    // Active scan actively queries devices for more info following detection.
    pBLEScan->setActiveScan(false);
    // pBLEScan->setInterval(97); // Select prime numbers to reduce risk of frequency beat pattern with ibeacon advertisement interval
    // pBLEScan->setWindow(37);   // Set to less or equal setInterval value. Leave reasonable gap to allow WiFi some time.

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
            if (pBLEScan->start(BLE_SCAN_TIME, false, true)) {
                retval = true; //Scan successfully started.
            } else {
                Log.verbose("Scan failed to start.\r\n");
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

uint8_t tiltScanner::load_tilt_from_advert_hex(const NimBLEAdvertisedDevice* advertisedDevice)
{
    uint8_t m_color;
    const std::string advert_string_hex = advertisedDevice->getManufacturerData();
    const int8_t current_rssi = advertisedDevice->getRSSI();

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
            strncat(m_color_arr, hex_code, 2);
        
        //Indices 20-21 each generate two characters of the temperature array
        if (i == 20 || i == 21)
            strncat(temp_arr, hex_code, 2);
        
        //Indices 22-23 each generate two characters of the sp_gravity array
        if (i == 22 || i == 23)
            strncat(grav_arr, hex_code, 2);
        
        //Index 24 contains the tx_pwr (which is used by recent tilts to indicate battery age)
        if (i == 24)
            strncat(tx_pwr_arr, hex_code, 2);
        
    }

    m_color = tiltHydrometer::uuid_to_color_no(m_color_arr);
    if (m_color == TILT_NONE) // We didn't match the uuid to a color (should only happen if new colors are released)
        return TILT_NONE;

    uint16_t temp = std::strtoul(temp_arr, nullptr, 16);
    uint16_t gravity = std::strtoul(grav_arr, nullptr, 16);
    uint8_t tx_pwr = std::strtoul(tx_pwr_arr, nullptr, 16);

    tiltHydrometer *th = get_or_create_tilt(advertisedDevice->getAddress(), m_color);
    th->set_values(temp, gravity, tx_pwr, current_rssi);
    th->m_address = advertisedDevice->getAddress();

    return m_color;
}



JsonDocument tiltScanner::tilt_to_json()
{
    tilt_scanner.drop_expired_tilts();
    
    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();

    for(tiltHydrometer & th : m_tilt_devices) {
        array.add(th.to_json(false));
    }

    return doc;
}


void tiltScanner::tilt_to_json_legacy(JsonDocument &doc)
{
    // This is only used for sending to Legacy Fermentrack
    // The main difference is that we don't return an array, but instead return a dict with the tilt color as the key
    // This is deprecated, and will be removed in the future.
    tilt_scanner.drop_expired_tilts();

    for(tiltHydrometer & th : m_tilt_devices) {
        // tilt_data[0] = {'\0'};
        // The other difference is that we send a "gravity" key in the JSON which contains the uncalibrated, smoothed gravity value
        JsonDocument tilt_data = th.to_json(true);
        doc[tilt_color_names[th.m_color]] = tilt_data;
    }
}


std::size_t tiltScanner::tilt_count() {
    return m_tilt_devices.size();
}


tiltHydrometer* tiltScanner::get_tilt(const NimBLEAddress devAddress, uint8_t color) {
    for(tiltHydrometer & th : m_tilt_devices) {
        if(th.m_address == devAddress && th.m_color == color) {
            // Access the object through the iterator
            return &th;
        }
    }
    return nullptr;
}


tiltHydrometer* tiltScanner::get_or_create_tilt(const NimBLEAddress devAddress, uint8_t color) {
    tiltHydrometer *found_th = get_tilt(devAddress, color);

    if(found_th)
        return found_th;

    // No matching device was found
    tiltHydrometer newTilt(devAddress, color);
    m_tilt_devices.push_front(newTilt);

    return get_tilt(devAddress, color);  // We specifically want to access the object as referenced in the list
}


void tiltScanner::drop_expired_tilts() {
    for (auto it = m_tilt_devices.begin(); it != m_tilt_devices.end(); ) {
        if (it->expired()) {
            Log.verbose("Dropping Tilt %s due to inactivity.\r\n", tilt_color_names[it->m_color]);
            it = m_tilt_devices.erase(it);
        } else {
            ++it;
        }
    }
}