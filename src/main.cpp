//
// Created by John Beeler on 4/26/18.
//

//Advertised Device: Name: Tilt, Address: 88:c2:55:ac:26:81, manufacturer data: 4c000215a495bb40c5b14b44b5121370f02d74de005004d9c5
//4c000215a495bb40c5b14b44b5121370f02d74de005004d9c5
//????????iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiittttgggg??
//**********----------**********----------**********


#include <nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;


#include "tiltBridge.h"
//#include "tilt/tiltHydrometer.h"
//#include "tilt/tiltScanner.h"

#include <Arduino.h>
//#include "bridge_lcd.h"
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

jsonConfigHandler app_config;

void setup() {
    Serial.begin(115200);
    Serial.println("Initializing...");

    // Initialize the BLE scanner
    tilt_scanner.init();

    // Intialize the display
    lcd.init();

    // Display the Fermentrack logo
    lcd.display_logo();
//    delay(5000);

    Serial.println("Initializing Config...");
    app_config.initialize();
    Serial.println(app_config.config.dump().c_str());

    Serial.println("Loading Config...");
    app_config.load();

//    app_config.config["didItWork"] = "Yes";
//    app_config.save();
    Serial.println(app_config.config.dump().c_str());

}

void loop() {

    tilt_scanner.scan();
    Serial.println("Async scan started, sleeping until scan completes...");
    tilt_scanner.wait_until_scan_complete();

//    BLEScanResults foundDevices = pBLEScan->start(scanTime);
    Serial.printf("RAM left %d\r\n", esp_get_free_heap_size());
    Serial.println(tilt_scanner.tilt_to_json().dump().c_str());
    lcd.display_tilts();
    delay(100);
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

