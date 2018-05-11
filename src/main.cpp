//
// Created by John Beeler on 4/26/18.
//
#include <Arduino.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager


/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
*/

//Advertised Device: Name: Tilt, Address: 88:c2:55:ac:26:81, manufacturer data: 4c000215a495bb40c5b14b44b5121370f02d74de005004d9c5
//4c000215a495bb40c5b14b44b5121370f02d74de005004d9c5
//????????iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiittttgggg??
//**********----------**********----------**********

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include "tiltHydrometer.h"

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

BLEScan* pBLEScan;
void setup() {
    Serial.begin(115200);
    Serial.println("Scanning...");

    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan(); //create new scan

//    MyAdvertisedDeviceCallbacks myCallbacks;
//    pBLEScan->setAdvertisedDeviceCallbacks(&myCallbacks);

    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());

    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
}

void loop() {
    // put your main code here, to run repeatedly:
    Serial.println("Starting scan!");
    BLEScanResults foundDevices = pBLEScan->start(scanTime);
    Serial.printf("RAM left %d\r\n", esp_get_free_heap_size());
//    Serial.print("Devices found: ");
//    Serial.println(foundDevices.getCount());
//    Serial.println("Scan done!");
    delay(5000);
}






//
//
//
//#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
//
//const char* modes[] = { "NULL", "STA", "AP", "STA+AP" };
//
//
//void setup() {
//    // put your setup code here, to run once:
//    Serial.begin(115200);
//    Serial.println("\n Starting");
//
//    Serial.setDebugOutput(true);
//    Serial.println(modes[WiFi.getMode()]);
//    WiFi.printDiag(Serial);
//
//    //Local intialization. Once its business is done, there is no need to keep it around
//    WiFiManager wifiManager;
//    //reset settings - for testing
//    //wifiManager.resetSettings();
//
//    //sets timeout until configuration portal gets turned off
//    //useful to make it all retry or go to sleep
//    //in seconds
//    wifiManager.setConfigPortalTimeout(180);
//
//    //fetches ssid and pass and tries to connect
//    //if it does not connect it starts an access point with the specified name
//    //here  "AutoConnectAP"
//    //and goes into a blocking loop awaiting configuration
//    if(!wifiManager.autoConnect("AutoConnectAP","bridge")) {
//        Serial.println("failed to connect and hit timeout");
//    }
//
//}
//
//
//void loop() {
//    // is configuration portal requested?
//    if (true) {
//        //WiFiManager
//        //Local intialization. Once its business is done, there is no need to keep it around
//        WiFiManager wifiManager;
//
//        //reset settings - for testing
//        //wifiManager.resetSettings();
//
//        //sets timeout until configuration portal gets turned off
//        //useful to make it all retry or go to sleep
//        //in seconds
//        wifiManager.setConfigPortalTimeout(120);
//
//        //it starts an access point with the specified name
//        //here  "AutoConnectAP"
//        //and goes into a blocking loop awaiting configuration
//
//        //WITHOUT THIS THE AP DOES NOT SEEM TO WORK PROPERLY WITH SDK 1.5 , update to at least 1.5.1
//        //WiFi.mode(WIFI_STA);
//
//        // disable captive portal redirection
//        // wifiManager.setCaptivePortalEnable(false);
//
//        if (!wifiManager.startConfigPortal("OnDemandAP")) {
//            Serial.println("failed to connect and hit timeout");
//            delay(3000);
//        } else {
//            //if you get here you have connected to the WiFi
//            Serial.println("connected...yeey :)");
//        }
//    }
//
//    // put your main code here, to run repeatedly:
//}

