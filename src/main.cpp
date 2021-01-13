//
// Created by John Beeler on 4/26/18.
//

#include "main.h"

Ticker memCheck;
uint64_t restart_time = 0;

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

    FILESYSTEM.begin();

    Log.verbose(F("Loading config." CR));
    loadConfig();

    Log.verbose(F("Initializing LCD." CR));
    // Handle setting the display up
    lcd.init(); // Initialize the display

    Log.verbose(F("Initializing WiFi." CR));
    init_wifi(); // Initialize WiFi (including configuration AP if necessary)
    initWiFiResetButton();

    // I kind of want to leave the WiFi info on screen longer here instead of the logo. The logo will display often
    // enough as-is.
    // lcd.display_logo();  // Display the logo

#ifdef LOG_LOCAL_LEVEL
    esp_log_level_set("*", ESP_LOG_DEBUG);        // Det all components to DEBUG level
    esp_log_level_set("wifi", ESP_LOG_WARN);      // Enable WARN logs from WiFi stack
    esp_log_level_set("dhcpc", ESP_LOG_WARN);     // Enable WARN logs from DHCP client
#endif

    // Initialize the BLE scanner
    tilt_scanner.init();
    tilt_scanner.scan();
    data_sender.init();      // Initialize the data sender
    data_sender.init_mqtt(); //Initialize the mqtt server connection if configured.

    // Once all this is done, we'll wait until the initial scan completes.
    tilt_scanner.wait_until_scan_complete();
    http_server.init();

    memCheck.attach(30, printMem);  // Memory debug print
}

void loop()
{
    serialLoop();   // Service telnet and console commands

    // The scans are done asynchronously, so we'll poke the scanner to see if a new scan needs to be triggered.
    if (tilt_scanner.scan())
    {
        // If we need to do anything when a new scan is started, trigger it here.
    }

    // handle_wifi_reset_presses();
    reconnectIfDisconnected(); // If we disconnected from the WiFi, attempt to reconnect
    data_sender.process();

    lcd.check_screen();

    if (http_server.wifireset_requested)
    {
        Log.verbose(F("Resetting WiFi." CR));
        http_server.wifireset_requested = false;
        tilt_scanner.wait_until_scan_complete(); // Wait for scans to complete (we don't want any tasks running in the background)
        vTaskDelay(3000);
        disconnect_from_wifi_and_restart();
    }

    if (http_server.restart_requested)
    {
        Log.verbose(F("Resetting controller." CR));
        http_server.restart_requested = false;
        tilt_scanner.wait_until_scan_complete(); // Wait for scans to complete (we don't want any tasks running in the background)
        vTaskDelay(3000);
        ESP.restart();                           // Restart the TiltBridge
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

    yield();
}
