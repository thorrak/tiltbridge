// TiltBridge is a Tilt-Hydrometer-to-WiFi Bridge
// Please note - This source code (along with other files) are provided under license.
// More details (including license details) can be found in the files accompanying this source code.

#include <esp_system.h>
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <nvs_flash.h>
#include <esp_bus.h>

#include <thorlog.h>

#include "filesystem.h"


#include "watchButtons.h"
#include "tilt/tiltScanner.h"
#include "http_server.h"
#include "wifi_setup.h"
#include "mdns_setup.h"
#include "sendData.h"
#include "jsonconfig.h"
#include "bridge_lcd.h"
#include "serialhandler.h"
#include "main.h"



#if (ARDUINO_LOG_LEVEL >= ARDUINO_LOG_LOG_LEVEL_INFO) && !defined(DISABLE_LOGGING)
TimerHandle_t memCheckTimer = nullptr;
#endif

TimerHandle_t reboot24Timer = nullptr;

// Timer callback for memory debug printing
#if (ARDUINO_LOG_LEVEL >= ARDUINO_LOG_LOG_LEVEL_INFO) && !defined(DISABLE_LOGGING)
static void memCheckTimerCallback(TimerHandle_t xTimer) {
    const uint32_t free = esp_get_free_heap_size();
    const uint32_t max = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    const uint8_t frag = (free > 0) ? (100 - (max * 100) / free) : 0;
    Log.info("Free Heap: %d, Largest contiguous block: %d, Frag: %d%%\r\n", free, max, frag);
}
#endif

// Timer callback for 24-hour reboot
static void reboot24TimerCallback(TimerHandle_t xTimer) {
    Log.notice("Rebooting on 24-hour timer." CR);
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
}

void printMem() {
    const uint32_t free = esp_get_free_heap_size();
    const uint32_t max = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    const uint8_t frag = (free > 0) ? (100 - (max * 100) / free) : 0;
    Log.info("Free Heap: %d, Largest contiguous block: %d, Frag: %d%%\r\n", free, max, frag);
}

void reboot()
{
    Log.notice("Rebooting on 24-hour timer." CR);
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
}

void setup() {

    esp_log_level_set("esp_bus", ESP_LOG_VERBOSE);

    // Initialize esp_bus (required for esp_wifi_manager events)
    ESP_LOGI("tiltbridge", "Initializing esp_bus.");
    ESP_ERROR_CHECK(esp_bus_init());

    serial();

    Log.verbose("Loading config.\r\n");
    // Initialize the filesystem 
    // (reformat if unable to initialize, though this will present broader problems as we won't have the web interface)
    if (!filesystem_init(true)) {
        Log.error("Unable to initialize filesystem.\r\n");
    }
    config.load();

    Log.verbose("Initializing LCD.\r\n");
    lcd.init();
    lcd.display_logo();

    // Initialize NVS (required for esp_wifi_manager)
    esp_err_t nvs_ret = nvs_flash_init();
    if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES || nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        Log.warning("NVS partition was truncated, erasing and reinitializing.\r\n");
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_ret);



    Log.info("Initializing WiFi.\r\n");
    initWiFi();

    Log.info("Initializing scanner.\r\n");
    tilt_scanner.init();                        // Initialize the BLE scanner
    tilt_scanner.wait_until_scan_complete();    // Wait until the initial scan completes

    data_sender.init();     // Initialize the data sender
    initButtons();          // Initialize buttons

    // Start independent timers using FreeRTOS software timers
    // ARDUINO_LOG_LOG_LEVEL_INFO is 4
#if (ARDUINO_LOG_LEVEL >= ARDUINO_LOG_LOG_LEVEL_INFO) && !defined(DISABLE_LOGGING)
    // Create periodic timer for memory debug printing (30 seconds)
    memCheckTimer = xTimerCreate("MemCheck", pdMS_TO_TICKS(30000), pdTRUE, nullptr, memCheckTimerCallback);
    if (memCheckTimer != nullptr) {
        xTimerStart(memCheckTimer, 0);
    }
#endif

    // Set a reboot timer for 24 hours (currently disabled)
    // reboot24Timer = xTimerCreate("Reboot24", pdMS_TO_TICKS(86400000), pdFALSE, nullptr, reboot24TimerCallback);
    // if (reboot24Timer != nullptr) {
    //     xTimerStart(reboot24Timer, 0);
    // }

}

void loop() {
    // These processes take precedence
    checkButtons();     // Check for reset calls

    data_sender.process();

    if (tilt_scanner.scan()) {
        // The scans are done asynchronously, so we'll poke the scanner to see if
        // a new scan needs to be triggered.

        // If we need to do anything when a new scan is started, trigger it here.
    }

    // Check semaphores

    if (doBoardReset || http_server.restart_requested) {
        Log.verbose("Resetting controller.\r\n");
        http_server.restart_requested = false;
        tilt_scanner.wait_until_scan_complete(); // Wait for scans to complete
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();                           // Restart the TiltBridge
    }

    if (doWiFiReset || http_server.wifi_reset_requested) {
        Log.verbose("Resetting WiFi configuration.\r\n");
        http_server.wifi_reset_requested = false;
        tilt_scanner.wait_until_scan_complete(); // Wait for scans to complete
        vTaskDelay(pdMS_TO_TICKS(1000));
        doWiFiReset = false;
        disconnectWiFi();
    }

    if (http_server.name_reset_requested) {
        Log.verbose("Resetting host name.\r\n");
        http_server.name_reset_requested = false;
        mdnsReset();
    }

    if (http_server.factoryreset_requested) {
        Log.verbose("Resetting to original settings.\r\n");
        http_server.factoryreset_requested = false;
        tilt_scanner.wait_until_scan_complete();    // Wait for scans to complete
        config.deleteFile();                        // Delete the config file in the filesystem
        disconnectWiFi();                           // Clear wifi config and restart
    }

    if (http_server.mqtt_init_rqd) {
        Log.verbose("Re-initializing MQTT.\r\n");
        http_server.mqtt_init_rqd = false;
        data_sender.init_mqtt();
    }

    if (http_server.lcd_reinit_rqd) {
        Log.verbose("Re-initializing LCD.\r\n");
        http_server.lcd_reinit_rqd = false;
        lcd.reinit();
    }

    reconnectWiFi();

    screenFlip(); // This must be in the loop
}

// Main loop task for FreeRTOS
static void loopTask(void* pvParameters) {
    for (;;) {
        loop();
        vTaskDelay(pdMS_TO_TICKS(10));  // Small delay to allow other tasks to run
    }
}

// ESP-IDF entry point
extern "C" void app_main(void) {
    setup();

    // Create the main loop task
    xTaskCreatePinnedToCore(
        loopTask,       // Task function
        "loopTask",     // Task name
        8192,           // Stack size (bytes)
        nullptr,        // Task parameters
        1,              // Priority
        nullptr,        // Task handle
        1               // Core ID (run on core 1, leaving core 0 for WiFi/BLE)
    );
}
