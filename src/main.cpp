//
// Created by John Beeler on 4/26/18.
//

#include "main.h"

#if (ARDUINO_LOG_LEVEL >= 5)
Ticker memCheck;
#endif
Ticker dataSend;
Ticker wifiCheck;
Ticker tiltScan;

void printMem()
{
    const uint32_t free = ESP.getFreeHeap();
    const uint32_t max = ESP.getMaxAllocHeap();
    const uint8_t frag = 100 - (max * 100) / free;
    Log.verbose(F("Free Heap: %d, Max Allocated: %d, Frag: %d%" CR), free, max, frag);
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
    esp_log_level_set("*", ESP_LOG_DEBUG);        // Set all components to debug level
    esp_log_level_set("wifi", ESP_LOG_WARN);      // Enable WARN logs from WiFi stack
    esp_log_level_set("dhcpc", ESP_LOG_WARN);     // Enable WARN logs from DHCP client
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
    dataSend.attach(1, dataDispatch);           // Send data
    wifiCheck.attach(1, reconnectWiFi);         // Check on WiFi
    tiltScan.attach(1, pingScanner);            // Nudge the Tilt scanner
}

void loop()
{
    // These processes take precedence
    serialLoop();       // Service telnet and console commands
    checkButtons();     // Check for reset calls

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
