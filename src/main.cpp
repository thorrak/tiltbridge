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
const char* modes[] = { "NULL", "STA", "AP", "STA+AP" };

jsonConfigHandler app_config;
uint64_t status_counter = 0;

#define DEBUG_PRINTS 1


bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback() {
//    Serial.println("Should save config");
    shouldSaveConfig = true;
}

// callback to display the WiFi LCD notification
void configModeCallback(WiFiManager *myWiFiManager) {
#ifdef DEBUG_PRINTS
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    Serial.println(myWiFiManager->getConfigPortalSSID());
#endif
    // Assuming WIFI_SETUP_AP_PASS here.
    lcd.display_wifi_connect_screen(myWiFiManager->getConfigPortalSSID(), WIFI_SETUP_AP_PASS);

}

// Not sure if this is sufficient to test for validity
bool isValidmDNSName(String mdns_name) {
    for (std::string::size_type i = 0; i < mdns_name.length(); ++i) {
        // For now, we're just checking that every character in the string is alphanumeric. May need to add more validation here.
        if (!isalnum(mdns_name[i]))
            return false;
    }
    return true;
}


void setup() {
    WiFiManager wifiManager;  //Local initialization. Once its business is done, there is no need to keep it around

    Serial.begin(115200);

    // Handle all of the config initialization & loading
#ifdef DEBUG_PRINTS
    Serial.println("Initializing Config...");
#endif
    app_config.initialize();

#ifdef DEBUG_PRINTS
    Serial.println(app_config.config.dump().c_str());
    Serial.println("Loading Config...");
#endif
    app_config.load();
#ifdef DEBUG_PRINTS
    Serial.println(app_config.config.dump().c_str());
#endif

    // Handle setting the display up
    lcd.init();  // Intialize the display


#ifdef DEBUG_PRINTS
    Serial.setDebugOutput(true);
    Serial.println(modes[WiFi.getMode()]);
    WiFi.printDiag(Serial);
#else
    wifiManager.setDebugOutput(FALSE); // In case we have a serial connection to BrewPi
#endif

    //reset settings - for testing
    //wifiManager.resetSettings();

    //sets timeout until configuration portal gets turned off
    //useful to make it all retry or go to sleep
    //in seconds
    wifiManager.setConfigPortalTimeout(5 * 60);  // Setting to 5 mins

    // The main purpose of this is to set a boolean value which will allow us to know we
    // just saved a new configuration (as opposed to rebooting normally)
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.setAPCallback(configModeCallback);

    // The third parameter we're passing here (mdns_id.c_str()) is the default name that will appear on the form.
    // It's nice, but it means the user gets no actual prompt for what they're entering.
    std::string mdns_id = app_config.config["mdnsID"];
    WiFiManagerParameter custom_mdns_name("mdns", "Device (mDNS) Name", mdns_id.c_str(), 20);
    wifiManager.addParameter(&custom_mdns_name);

    std::string fermentrack_url = app_config.config["fermentrackURL"];
    WiFiManagerParameter custom_fermentrack_url("ferm_url", "Fermentrack Target URL", fermentrack_url.c_str(), 128);
    wifiManager.addParameter(&custom_fermentrack_url);


    if(wifiManager.autoConnect(WIFI_SETUP_AP_NAME, WIFI_SETUP_AP_PASS)) {
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_AP_STA);
    } else {
        Serial.println("failed to connect and hit timeout");
        lcd.display_wifi_fail_screen();
        delay(2 * 60 * 1000);
        ESP.restart();
        // TODO - Determine if we should hang here, force restart, or what.
    }

    // Alright. We're theoretically connected here.
    // If we connected, then let's save the mDNS name
    if (shouldSaveConfig) {
        // If the mDNS name is valid, save it.
        if (isValidmDNSName(custom_mdns_name.getValue())) {
            app_config.config["mdnsID"] = custom_mdns_name.getValue();
        } else {
            // If the mDNS name is invalid, reset the WiFi configuration and restart the ESP8266
            // TODO - add an LCD error message here maybe
            WiFi.disconnect(true);
            delay(2000);
            ESP.restart();
        }

        app_config.config["fermentrackURL"] = custom_fermentrack_url.getValue();
        app_config.save();
    }

    lcd.display_logo();  // Display the Fermentrack logo

    // Initialize the BLE scanner
    tilt_scanner.init();
    tilt_scanner.scan();

#ifdef DEBUG_PRINTS
    Serial.println("Initial scan started, sleeping until scan completes...");
#endif

    tilt_scanner.wait_until_scan_complete();

}

void loop() {
#ifdef DEBUG_PRINTS
    if(tilt_scanner.scan())
        Serial.println("Async scan started...");
#else
    tilt_scanner.scan();
#endif

    if(status_counter <= xTaskGetTickCount()) {
        // Every 10 seconds, print some kind of status
#ifdef DEBUG_PRINTS
        Serial.printf("RAM left %d\r\n", esp_get_free_heap_size());
        Serial.println(tilt_scanner.tilt_to_json().dump().c_str());
#endif
        status_counter = xTaskGetTickCount() + 10000;
    }
//    BLEScanResults foundDevices = pBLEScan->start(scanTime);
    lcd.check_screen();
}
