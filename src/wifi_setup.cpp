//
// Created by John Beeler on 6/4/18.
//

#include "wifi_setup.h"




#include <nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;


#include "tiltBridge.h"
#include <Arduino.h>
//#include "bridge_lcd.h"
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
//const char* modes[] = { "NULL", "STA", "AP", "STA+AP" };
#include <ESPmDNS.h>
#include <WiFiClient.h>



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




void init_wifi() {
#ifndef OPTIONAL_WIFI
    WiFiManager wifiManager;  //Local initialization. Once its business is done, there is no need to keep it around
    wifiManager.setDebugOutput(false); // In case we have a serial connection to BrewPi
    //reset settings - for testing
    //wifiManager.resetSettings();

    //sets timeout until configuration portal gets turned off
    //useful to make it all retry or go to sleep
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

    std::string password = app_config.config["password"];
    WiFiManagerParameter custom_password("password", "TiltBridge Password", password.c_str(), 128);
    wifiManager.addParameter(&custom_password);


    if(wifiManager.autoConnect(WIFI_SETUP_AP_NAME, WIFI_SETUP_AP_PASS)) {
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_AP_STA);
    } else {
        Serial.println("failed to connect and hit timeout");
        lcd.display_wifi_fail_screen();
        delay(1 * 60 * 1000);
        ESP.restart();
        // TODO - Determine if we should hang here, force restart, or what.
    }

    // Alright. We're theoretically connected here.
    // If we connected, then let's save the mDNS name
    if (shouldSaveConfig) {
        // If the mDNS name is valid, save it.
        if (isValidmDNSName(custom_mdns_name.getValue())) {
            app_config.config["mdnsID"] = custom_mdns_name.getValue();
            mdns_id = app_config.config["mdnsID"];
        } else {
            // If the mDNS name is invalid, reset the WiFi configuration and restart the ESP8266
            // TODO - add an LCD error message here maybe
            WiFi.disconnect(true);
            delay(2000);
            ESP.restart();
        }

        // TODO - Validate the password
        app_config.config["password"] = custom_password.getValue();
        app_config.save();
    }

    if (!MDNS.begin(mdns_id.c_str())) {
        Serial.println("Error setting up MDNS responder!");
    }
    Serial.println("mDNS responder started");
    MDNS.addService("http", "tcp", 80);  // technically we should wait on this, but I'm impatient.

#endif
}

