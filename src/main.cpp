//
// Created by John Beeler on 4/26/18.
// Modified by Tim Pletcher on 31-Oct-2020.
//


//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
//#include "esp_log.h"

//#include <nlohmann/json.hpp>

// for convenience
//using json = nlohmann::json;
#include <ArduinoJson.h>

#include "tiltBridge.h"
#include "wifi_setup.h"
#include <Arduino.h>
#include "FS.h"
#include "SPIFFS.h"
//#include "bridge_lcd.h"
#ifdef DEBUG_PRINTS
//#include <AsyncWiFiManager.h> // https://github.com/tzapu/WiFiManager
#endif

#include "http_server.h"
#include "sendData.h"

jsonConfigHandler app_config;
#ifdef DEBUG_PRINTS
uint64_t trigger_next_data_send = 0;
#endif
uint64_t restart_time = 0;

#include <driver/i2c.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include "sdkconfig.h"


void setup() {
#ifdef DEBUG_PRINTS
    //Serial.begin(115200);
    //Serial.setDebugOutput(false);
#endif
    // Handle all of the config initialization & loading
#ifdef DEBUG_PRINTS
    Serial.println("Initializing Config...");
#endif
    SPIFFS.begin(true);

#ifdef DEBUG_PRINTS
    Serial.println("Loading Config...");
#endif
    app_config.load();
#ifdef DEBUG_PRINTS
    char * config_js = (char *) malloc(sizeof(char) * 2500);
    app_config.dump_config(config_js);
    free(config_js);
    Serial.println(config_js);
#endif
    
    Serial.println(F("init lcd"));
    // Handle setting the display up
    lcd.init();  // Intialize the display
    Serial.println(F("init wifi"));
    init_wifi();  // Initialize WiFi (including configuration AP if necessary)
    initWiFiResetButton();

    // I kind of want to leave the WiFi info on screen longer here instead of the logo. The logo will display often
    // enough as-is.
//    lcd.display_logo();  // Display the Fermentrack logo

//    esp_log_level_set("*", ESP_LOG_DEBUG);        // set all components to DEBUG level
//    esp_log_level_set("wifi", ESP_LOG_WARN);      // enable WARN logs from WiFi stack
//    esp_log_level_set("dhcpc", ESP_LOG_WARN);     // enable WARN logs from DHCP client

    // Initialize the BLE scanner
    tilt_scanner.init();
    tilt_scanner.scan();
    data_sender.init();  // Initialize the data sender
    data_sender.init_mqtt(); //Initialize the mqtt server connection if configured.

    // Once all this is done, we'll wait until the initial scan completes.
    tilt_scanner.wait_until_scan_complete();
    http_server.init();
}



void loop() {
    
    // The scans are done asynchronously, so we'll poke the scanner to see if a new scan needs to be triggered.
    if(tilt_scanner.scan()) {
        // If we need to do anything when a new scan is started, trigger it here.
    }

#ifdef DEBUG_PRINTS
    // This is optional & just allows us to print the available ram in case of memory leaks.
    if(trigger_next_data_send <= xTaskGetTickCount()) {  // Every 10 seconds, print some kind of status
        Serial.printf_P(PSTR("Current / Minimum RAM left:  %d  /  "), esp_get_free_heap_size());
        Serial.printf_P(PSTR("%d\r\n"), esp_get_minimum_free_heap_size());
        trigger_next_data_send = xTaskGetTickCount() + 10000;
    }
#endif

    //handle_wifi_reset_presses();
    reconnectIfDisconnected();  // If we disconnected from the WiFi, attempt to reconnect
    data_sender.process();
    lcd.check_screen();
    if (http_server.config_updated) {
        app_config.save();
        http_server.config_updated = false;
    }
    if (http_server.restart_requested){ // Restart handling put in main loop to ensure that client has opportunity 
                                        // to grab the new mDNS name from /settings/json/ before restart for proper redirect.
        if(restart_time <= xTaskGetTickCount()) {
            tilt_scanner.wait_until_scan_complete();    // Wait for scans to complete (we don't want any tasks running in the background)
            ESP.restart();         // Restart the TiltBridge
        }
    } else {
        restart_time = xTaskGetTickCount() + 5000;
    }
    if (http_server.mqtt_init_rqd) {
        data_sender.init_mqtt();
        http_server.mqtt_init_rqd = false;
    }
    if (http_server.lcd_init_rqd) {
        lcd.init();
    }

    yield();
    
}
