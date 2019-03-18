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
#ifdef DEBUG_PRINTS
uint64_t trigger_next_data_send = 0;
#endif


#include <driver/i2c.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include "sdkconfig.h"


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
    app_config.load();
#ifdef DEBUG_PRINTS
    Serial.println(app_config.config.dump().c_str());
#endif

    // Handle setting the display up
    lcd.init();  // Intialize the display

    Serial.setDebugOutput(false);

    init_wifi();  // Initialize WiFi (including configuration AP if necessary)
#ifdef DEBUG_PRINTS
    Serial.println("WiFi initialized...");
#endif

    // I kind of want to leave the WiFi info on screen longer here instead of the logo. The logo will display often
    // enough as-is.
//    lcd.display_logo();  // Display the Fermentrack logo

    // Initialize the BLE scanner
    tilt_scanner.init();
    tilt_scanner.scan();

    data_sender.init();  // Initialize the data sender

#ifdef DEBUG_PRINTS
    Serial.println("Initial scan started, sleeping until scan completes...");
#endif

    tilt_scanner.wait_until_scan_complete();
    http_server.init();
}



void loop() {
    // The scans are done asynchronously, so we'll poke the scanner to see if a new scan needs to be triggered.
    if(tilt_scanner.scan()) {
#ifdef DEBUG_PRINTS
        // Serial.println("Async scan started...");
#endif
    }

#ifdef DEBUG_PRINTS
    if(trigger_next_data_send <= xTaskGetTickCount()) {  // Every 10 seconds, print some kind of status
        Serial.printf("RAM left %d\r\n", esp_get_free_heap_size());
        trigger_next_data_send = xTaskGetTickCount() + 10000;
//        Serial.println(tilt_scanner.tilt_to_json().dump().c_str());
    }
#endif

    data_sender.process();
    lcd.check_screen();
    http_server.handleClient();
    yield();
}
