// TiltBridge is a Tilt-Hydrometer-to-WiFi Bridge
// Please note - This source code (along with other files) are provided under license.
// More details (including license details) can be found in the files accompanying this source code.

#include "watchButtons.h"
#include "main.h"
#include "tilt/tiltScanner.h"
#include "http_server.h"


#if (ARDUINO_LOG_LEVEL >= 5)
Ticker memCheck;
#endif

Ticker reboot24;

void printMem() {
    const uint32_t free = ESP.getFreeHeap();
    const uint32_t max = ESP.getMaxAllocHeap();
    const uint8_t frag = 100 - (max * 100) / free;
    Log.info(F("Free Heap: %d, Largest contiguous block: %d, Frag: %d%%\r\n"), free, max, frag);
}

void reboot()
{
    Log.notice(F("Rebooting on 24-hour timer." CR));
    delay(500);
    ESP.restart();
}

void setup() {
    serial();

    Log.verbose(F("Loading config.\r\n"));
    // Initialize the filesystem 
    // (reformat if unable to initialize, though this will present broader problems as we won't have the web interface)
    if (!FILESYSTEM.begin(true)) {
        Log.verbose(F("Unable to initialize filesystem.\r\n"));
    }
    config.load();

    Log.verbose(F("Initializing LCD.\r\n"));
    lcd.init();

    Log.verbose(F("Initializing WiFi.\r\n"));
    initWiFi();

    Log.verbose(F("Initializing scanner.\r\n"));
    tilt_scanner.init();                        // Initialize the BLE scanner
    tilt_scanner.wait_until_scan_complete();    // Wait until the initial scan completes

    data_sender.init();     // Initialize the data sender
    http_server.init();     // Initialize the web server
    initButtons();          // Initialize buttons

    // Start independent timers
    // ARDUINO_LOG_LOG_LEVEL_INFO is 4
#if (ARDUINO_LOG_LEVEL >= ARDUINO_LOG_LOG_LEVEL_INFO) && !defined(DISABLE_LOGGING)
    memCheck.attach(30, printMem);              // Memory debug print on timer
#endif

    // Set a reboot timer for 24 hours
    reboot24.once(86400, reboot);

    // Set up Parse
    doParsePoll();
}

void loop() {
    // These processes take precedence
    serialLoop();       // Service telnet and console commands
    checkButtons();     // Check for reset calls

    send_to_cloud();
    data_sender.send_to_localTarget();
    send_to_bf_and_bf();
    data_sender.send_to_brewstatus();
    data_sender.send_to_grainfather();
    data_sender.send_to_taplistio();
    data_sender.send_to_google();
    data_sender.send_to_mqtt();

    if (tilt_scanner.scan()) {
        // The scans are done asynchronously, so we'll poke the scanner to see if
        // a new scan needs to be triggered.

        // If we need to do anything when a new scan is started, trigger it here.
    }

    // Check semaphores

    if (doBoardReset || http_server.restart_requested) {
        Log.verbose(F("Resetting controller.\r\n"));
        http_server.restart_requested = false;
        tilt_scanner.wait_until_scan_complete(); // Wait for scans to complete
        delay(1000);
        ESP.restart();                           // Restart the TiltBridge
    }

    if (doWiFiReset || http_server.wifi_reset_requested) {
        Log.verbose(F("Resetting WiFi configuration.\r\n"));
        http_server.wifi_reset_requested = false; 
        tilt_scanner.wait_until_scan_complete(); // Wait for scans to complete
        delay(1000);
        doWiFiReset = false;
        disconnectWiFi();
    }

    if (http_server.name_reset_requested) {
        Log.verbose(F("Resetting host name.\r\n"));
        http_server.name_reset_requested = false;
        mdnsReset();
    }

    if (http_server.factoryreset_requested) {
        Log.verbose(F("Resetting to original settings.\r\n"));
        http_server.factoryreset_requested = false;
        tilt_scanner.wait_until_scan_complete();    // Wait for scans to complete
        config.deleteFile();                        // Delete the config file in SPIFFS
        disconnectWiFi();                           // Clear wifi config and restart
    }

    if (http_server.mqtt_init_rqd) {
        Log.verbose(F("Re-initializing MQTT.\r\n"));
        http_server.mqtt_init_rqd = false;
        data_sender.init_mqtt();
    }

    if (http_server.lcd_reinit_rqd) {
        Log.verbose(F("Re-initializing LCD.\r\n"));
        http_server.lcd_reinit_rqd = false;
        lcd.reinit();
    }

    reconnectWiFi();

    screenFlip(); // This must be in the loop
}
