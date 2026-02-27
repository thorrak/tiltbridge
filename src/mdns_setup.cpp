#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <mdns.h>
#include <esp_wifi_manager.h>

#include <thorlog.h>

#include "jsonconfig.h"
#include "wifi_setup.h"  // For WEB_SERVER_PORT
#include "mdns_setup.h"

void initMDNS() {
    if (mdns_init() != ESP_OK || mdns_hostname_set(config.mdnsID) != ESP_OK) {
        Log.error("Error setting up MDNS responder.\r\n");
    } else {
        Log.notice("mDNS responder started, hostname: %s.local\r\n", config.mdnsID);
    }

    mdns_service_add(NULL, "_http", "_tcp", WEB_SERVER_PORT, NULL, 0);
    mdns_service_add(NULL, "_tiltbridge", "_tcp", WEB_SERVER_PORT, NULL, 0);

    // Store the mDNS name in wifi_manager's custom variables for persistence
    wifi_manager_set_var("mdns_name", config.mdnsID);
}

void mdnsReset() {
    mdns_free();
    if (mdns_init() != ESP_OK || mdns_hostname_set(config.mdnsID) != ESP_OK) {
        Log.error("Error resetting MDNS responder.");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    } else {
        Log.notice("mDNS responder restarted, hostname: %s.local.\r\n", config.mdnsID);
        mdns_service_add(NULL, "_http", "_tcp", WEB_SERVER_PORT, NULL, 0);
        mdns_service_add(NULL, "_tiltbridge", "_tcp", WEB_SERVER_PORT, NULL, 0);
    }

    // Update the mDNS name in wifi_manager's custom variables
    wifi_manager_set_var("mdns_name", config.mdnsID);
}
