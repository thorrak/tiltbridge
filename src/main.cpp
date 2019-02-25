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
#include "wifi_setup.h"
#include <Arduino.h>
#include "FS.h"
#include "SPIFFS.h"
//#include "bridge_lcd.h"
#ifdef DEBUG_PRINTS
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#endif

#include "http_server.h"
#include "sendData.h"

jsonConfigHandler app_config;
uint64_t trigger_next_data_send = 0;




void setup() {
    Serial.begin(115200);

    // Handle all of the config initialization & loading
#ifdef DEBUG_PRINTS
    Serial.println("Initializing Config...");
#endif
    app_config.initialize();
    SPIFFS.begin(true);

#ifdef DEBUG_PRINTS
    Serial.println(app_config.config.dump().c_str());
    Serial.println("Loading Config...");
#endif
//    app_config.load();
#ifdef DEBUG_PRINTS
    Serial.println(app_config.config.dump().c_str());
#endif

    // Handle setting the display up
    lcd.init();  // Intialize the display


    Serial.setDebugOutput(false);

    init_wifi();  // Initialize WiFi (including configuration AP if necessary)
    Serial.println("WiFi initialized...");
    setClock();

    lcd.display_logo();  // Display the Fermentrack logo
    Serial.println("Logo displayed...");

    // Initialize the BLE scanner
    tilt_scanner.init();
    tilt_scanner.scan();

#ifdef DEBUG_PRINTS
    Serial.println("Initial scan started, sleeping until scan completes...");
#endif

    tilt_scanner.wait_until_scan_complete();
    http_server.init();
#ifdef USE_SECURE_GSCRIPTS
    prep_send_secure();
#endif
}



void loop() {
    // The scans are done asynchronously, so we'll poke the scanner to see if a new scan needs to be triggered.
    if(tilt_scanner.scan()) {
#ifdef DEBUG_PRINTS
        Serial.println("Async scan started...");
#endif
    }

    // trigger_next_data_send is the
    if(trigger_next_data_send <= xTaskGetTickCount()) {
#ifdef DEBUG_PRINTS
        // Every 10 seconds, print some kind of status
        Serial.printf("RAM left %d\r\n", esp_get_free_heap_size());
//        Serial.println(tilt_scanner.tilt_to_json().dump().c_str());
#endif


        if(WiFi.status()== WL_CONNECTED && app_config.config["fermentrackURL"].get<std::string>().length() > 12) {   //Check WiFi connection status

            tilt_scanner.wait_until_scan_complete();
            send_to_fermentrack();
#ifdef USE_SECURE_GSCRIPTS
            send_secure();
#endif

            trigger_next_data_send = xTaskGetTickCount() + 10000;
        }


    }

    lcd.check_screen();
    http_server.handleClient();
    yield();
}
