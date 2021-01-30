//
// Created by John Beeler on 4/26/18.
//

#include "main.h"

#if (ARDUINO_LOG_LEVEL >= 5)
Ticker memCheck;
#endif

void printMem()
{
    const uint32_t free = ESP.getFreeHeap();
    const uint32_t max = ESP.getMaxAllocHeap();
    const uint8_t frag = 100 - (max * 100) / free;
    Log.verbose(F("Free Heap: %d, Max Allocated: %d, Frag: %d" CR), free, max, frag);
}

void setup()
{
    serial();

    Log.verbose(F("Loading config." CR));
    loadConfig();

    Log.verbose(F("Initializing LCD." CR));
    lcd.init();

    Log.verbose(F("Initializing WiFi." CR));
    initWiFi();

#ifdef LOG_LOCAL_LEVEL
    esp_log_level_set("*", ESP_LOG_VERBOSE);

    esp_log_level_set("FreeRTOS", ESP_LOG_WARN);
    esp_log_level_set("NimBLE", ESP_LOG_WARN);
    esp_log_level_set("NIMBLE_NVS", ESP_LOG_WARN);
    esp_log_level_set("NimBLEAddress", ESP_LOG_WARN);
    esp_log_level_set("NimBLEAdvertisedDevice", ESP_LOG_WARN);
    esp_log_level_set("NimBLEAdvertising", ESP_LOG_WARN);
    esp_log_level_set("NimBLEAdvertisingReport", ESP_LOG_WARN);
    esp_log_level_set("NimBLEBeacon", ESP_LOG_WARN);
    esp_log_level_set("NimBLECharacteristic", ESP_LOG_WARN);
    esp_log_level_set("NimBLEClient", ESP_LOG_WARN);
    esp_log_level_set("NimBLEDescriptor", ESP_LOG_WARN);
    esp_log_level_set("NimBLEDevice", ESP_LOG_WARN);
    esp_log_level_set("NimBLEEddystoneTLM", ESP_LOG_WARN);
    esp_log_level_set("NimBLEEddystoneURL", ESP_LOG_WARN);
    esp_log_level_set("NimBLERemoteCharacteristic", ESP_LOG_WARN);
    esp_log_level_set("NimBLERemoteDescriptor", ESP_LOG_WARN);
    esp_log_level_set("NimBLERemoteService", ESP_LOG_WARN);
    esp_log_level_set("NimBLEScan", ESP_LOG_WARN);
    esp_log_level_set("NimBLEServer", ESP_LOG_WARN);
    esp_log_level_set("NimBLEService", ESP_LOG_WARN);
    esp_log_level_set("NimBLEUtils", ESP_LOG_WARN);
    esp_log_level_set("NimBLEUUID", ESP_LOG_WARN);
    
    esp_log_level_set("wifi", ESP_LOG_VERBOSE);      // Enable WARN logs from WiFi stack
    esp_log_level_set("dhcpc", ESP_LOG_VERBOSE);     // Enable WARN logs from DHCP client
#endif

    Log.verbose(F("Initializing scanner." CR));
    tilt_scanner.init();                        // Initialize the BLE scanner
    tilt_scanner.wait_until_scan_complete();    // Wait until the initial scan completes

    data_sender.init();     // Initialize the data sender
    http_server.init();     // Initialize the web server
    initButtons();          // Initialize buttons

    // Start independent timers
#if (ARDUINO_LOG_LEVEL >= 5)
    memCheck.attach(30, printMem);              // Memory debug print on timer
#endif
}

void loop()
{
    // These processes take precedence
    serialLoop();       // Service telnet and console commands
    checkButtons();     // Check for reset calls

    // data_sender.send_to_localTarget();
    // send_to_bf_and_bf();    // TODO: Need to test this well
    // data_sender.send_to_brewstatus();
    data_sender.send_to_google();
    // data_sender.send_to_mqtt();

    if (tilt_scanner.scan())
    {
        // The scans are done asynchronously, so we'll poke the scanner to see if
        // a new scan needs to be triggered.

        // If we need to do anything when a new scan is started, trigger it here.
    }

    // Check semaphores

    if (doBoardReset || http_server.restart_requested)
    {
        Log.verbose(F("Resetting controller." CR));
        http_server.restart_requested = false;
        tilt_scanner.wait_until_scan_complete(); // Wait for scans to complete
        vTaskDelay(1000);
        ESP.restart();                           // Restart the TiltBridge
    }

    if (doWiFiReset || http_server.wifi_reset_requested)
    {
        Log.verbose(F("Resetting WiFi configuration." CR));
        http_server.wifi_reset_requested = false; 
        tilt_scanner.wait_until_scan_complete(); // Wait for scans to complete
        vTaskDelay(1000);
        doWiFiReset = false;
        disconnectWiFi();
    }

    if (http_server.name_reset_requested)
    {
        Log.verbose(F("Resetting host name." CR));
        http_server.name_reset_requested = false;
        mdnsReset();
    }

    if (http_server.factoryreset_requested)
    {
        Log.verbose(F("Resetting to original settings." CR));
        http_server.factoryreset_requested = false;
        tilt_scanner.wait_until_scan_complete();    // Wait for scans to complete
        deleteConfigFile();                         // Delete the config file in SPIFFS
        disconnectWiFi();                           // Clear wifi config and restart
    }

    if (http_server.mqtt_init_rqd)
    {
        Log.verbose(F("Re-initializing MQTT." CR));
        http_server.mqtt_init_rqd = false;
        data_sender.init_mqtt();
    }

    if (http_server.lcd_reinit_rqd)
    {
        Log.verbose(F("Re-initializing LCD." CR));
        http_server.lcd_reinit_rqd = false;
        lcd.reinit();
    }

    screenFlip(); // This must be in the loop
}
