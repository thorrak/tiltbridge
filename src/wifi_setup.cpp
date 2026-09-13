#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <cstring>
#include <esp_timer.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <esp_event.h>
#include "mdns_setup.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <thorlog.h>
#include <esp_wifi_config.h>
#include <esp_log.h>

#include "url_utils.h"
#include "bridge_lcd.h"
#include "jsonconfig.h"  // For config struct and global instance
#include "idf_http_server.h"
#include "http_server.h"

#include "wifi_setup.h"

// Track WiFi connection state to distinguish initial connection from reconnection.
// This flag is set to true when WiFi disconnects and reset to false when reconnected.
// Used to determine appropriate LCD display: success screen (initial) vs logo (reconnection).
static bool wifi_was_disconnected = false;

// Event callback for WiFi connecting (attempting to connect to a network)
static void on_wifi_connecting(void *arg, esp_event_base_t base, int32_t event_id, void *data) {
    if (data == nullptr) {
        return;
    }
    const char *ssid = (const char *)data;
    Log.info("WiFi connecting to %s\r\n", ssid);

    // Don't clobber the AP screen with "connecting to..." during background reconnect attempts
    wifi_status_t status;
    if (wifi_cfg_get_status(&status) == ESP_OK && status.ap_active) {
        return;
    }

    lcd.display_wifi_connecting_screen(ssid);
}

// Event callback for WiFi connected
static void on_wifi_connected(void *arg, esp_event_base_t base, int32_t event_id, void *data) {
    if (data == nullptr) {
        Log.warning("WiFi connected event received with invalid payload\r\n");
        return;
    }
    const wifi_connected_t *info = (const wifi_connected_t *)data;
    Log.notice("WiFi connected to %s, channel %d, RSSI %d\r\n", info->ssid, info->channel, info->rssi);
}

// Event callback for WiFi got IP
static void on_wifi_got_ip(void *arg, esp_event_base_t base, int32_t event_id, void *data) {
    wifi_status_t status;
    if (wifi_cfg_get_status(&status) == ESP_OK) {
        Log.notice("WiFi got IP: %s\r\n", status.ip);

        if (wifi_was_disconnected) {
            // This is a reconnection after a disconnect
            Log.notice("Reconnected to WiFi after disconnect\r\n");
            mdnsReset();  // Re-establish mDNS services
            lcd.display_logo();
            wifi_was_disconnected = false;
        } else {
            // Initial connection - display success screen with access URLs
            char mdns_url[50] = "http://";
            strncat(mdns_url, config.mdnsID, 31);
            strcat(mdns_url, ".local/");

            char ip_address_url[25] = "http://";
            strncat(ip_address_url, status.ip, 16);
            strcat(ip_address_url, "/");

            lcd.display_wifi_success_screen(mdns_url, ip_address_url);
        }
    }
}

// Event callback for WiFi disconnected
static void on_wifi_disconnected(void *arg, esp_event_base_t base, int32_t event_id, void *data) {
    if (data == nullptr) {
        Log.warning("WiFi disconnected event received with invalid payload\r\n");
        return;
    }
    const wifi_disconnected_t *info = (const wifi_disconnected_t *)data;
    Log.warning("WiFi disconnected from %s, reason: %d. Auto-reconnect in progress.\r\n", info->ssid, info->reason);

    // Update state and LCD/UI to show the disconnected state
    wifi_was_disconnected = true;
    lcd.display_wifi_disconnected_screen();
}

// Event callback for AP started
static void on_wifi_ap_started(void *arg, esp_event_base_t base, int32_t event_id, void *data) {
    wifi_ap_status_t ap_status;
    Log.info("WiFi AP started for configuration.\r\n");
    if (wifi_cfg_get_ap_status(&ap_status) == ESP_OK) {
        Log.info("AP started: SSID: %s, IP: %s\r\n", ap_status.ssid, ap_status.ip);
        lcd.display_wifi_connect_screen(ap_status.ssid, WIFI_SETUP_AP_PASS);
        esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);  // Set the bandwidth of ESP32 interface
    }
}

// Event callback for provisioning stopped — initialize the HTTP server routes
static void on_provisioning_stopped(void *arg, esp_event_base_t base, int32_t event_id, void *data) {
    Log.info("WiFi provisioning stopped, initializing HTTP server.\r\n");
    http_server.init();
}

// Event callback for variable changes (e.g., mdns_name changed via WiFi config API)
static void on_var_changed(void *arg, esp_event_base_t base, int32_t event_id, void *data) {
    if (data == nullptr) {
        return;
    }
    const wifi_var_t *var = (const wifi_var_t *)data;

    if (strcmp(var->key, "mdns_name") == 0 && strlen(var->value) > 0) {
        if (isValidHostName(var->value) && strcmp(var->value, config.mdnsID) != 0) {
            Log.notice("mDNS name changed via WiFi config: %s\r\n", var->value);
            strlcpy(config.mdnsID, var->value, sizeof(config.mdnsID));
            config.save();
            mdnsReset();
        }
    }
}

void disconnectWiFi() {
    Log.notice("Resetting WiFi settings via disconnectWiFi()\r\n");
    wifi_cfg_disconnect();
    wifi_cfg_factory_reset();
    vTaskDelay(pdMS_TO_TICKS(1000));  // Give everything a moment to settle before resetting
    esp_restart();
}


void initWiFi() {

    esp_log_level_set("wifi_cfg", ESP_LOG_VERBOSE);
    esp_log_level_set("tiltbridge", ESP_LOG_VERBOSE);

    // Initialize TCP/IP stack before starting HTTP server
    // esp_netif_init() is safe to call multiple times
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default event loop if not already created
    esp_err_t evt_ret = esp_event_loop_create_default();
    if (evt_ret != ESP_OK && evt_ret != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(evt_ret);  // Only fail on unexpected errors
    }

    // Start HTTP server early so we can share it with wifi_cfg
    // This prevents port conflicts when wifi_cfg's HTTP server is torn down
    esp_err_t http_ret = idf_httpd_start();
    if (http_ret != ESP_OK) {
        Log.error("Failed to start HTTP server early: %s\r\n", esp_err_to_name(http_ret));
    }

    // Subscribe to WiFi events. esp_wifi_config (0.2.0+) posts these on the
    // default event loop (created above) under the WIFI_CFG_EVENT base.
    // Registering before wifi_cfg_init() ensures we catch startup events.
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_CFG_EVENT, WIFI_CFG_EVENT_CONNECTING, on_wifi_connecting, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_CFG_EVENT, WIFI_CFG_EVENT_CONNECTED, on_wifi_connected, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_CFG_EVENT, WIFI_CFG_EVENT_GOT_IP, on_wifi_got_ip, NULL));
    // ESP_ERROR_CHECK(esp_event_handler_register(WIFI_CFG_EVENT, WIFI_CFG_EVENT_DISCONNECTED, on_wifi_disconnected, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_CFG_EVENT, WIFI_CFG_EVENT_AP_START, on_wifi_ap_started, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_CFG_EVENT, WIFI_CFG_EVENT_VAR_CHANGED, on_var_changed, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_CFG_EVENT, WIFI_CFG_EVENT_PROVISIONING_STOPPED, on_provisioning_stopped, NULL));

    // Default variables for WiFi config - mdns_name is used to set the mDNS hostname
    // This provides a default value; if NVS has a stored value, that takes precedence
    static wifi_var_t default_vars[] = {
        {"mdns_name", "tiltbridge"},
    };

    // Configure WiFi Config. Start from the library defaults (required as of
    // esp_wifi_config 0.2.0: wifi_cfg_init() no longer patches unset fields and
    // rejects a zero retry backoff) and override only what TiltBridge needs.
    // Struct-value style rather than a designated initialiser: C++ requires
    // designators in declaration order, which WIFI_CFG_DEFAULTS + overrides
    // cannot satisfy.
    wifi_cfg_config_t wifi_config = WIFI_CFG_DEFAULT_CONFIG();

    wifi_config.default_vars = default_vars;
    wifi_config.default_var_count = sizeof(default_vars) / sizeof(default_vars[0]);

    // Retry policy: 3 attempts per network, 5 s backoff base, 60 s cap,
    // auto-reconnect on. These match the library defaults; stated explicitly.
    wifi_config.max_retry_per_network = 3;
    wifi_config.retry_interval_ms = 5000;
    wifi_config.retry_max_interval_ms = 60000;
    wifi_config.auto_reconnect = true;

    // If we fail to connect to any known network, start provisioning (SoftAP + captive portal + BLE)
    wifi_config.provisioning_mode = WIFI_PROV_ON_FAILURE;
    // Stop the AP and captive portal and deregister httpd endpoints once we successfully connect
    wifi_config.stop_provisioning_on_connect = true;
    wifi_config.provisioning_teardown_delay_ms = 5000;
    // Unregister captive portal/webui routes after provisioning so TiltBridge can register its own
    wifi_config.http_post_prov_mode = WIFI_HTTP_API_ONLY;

    // SoftAP for the captive portal. Only SSID, password and channel differ from
    // the library defaults (192.168.4.1/24, DHCP .2-.20, 4 clients, not hidden).
    strlcpy(wifi_config.default_ap.ssid, WIFI_SETUP_AP_NAME, sizeof(wifi_config.default_ap.ssid));
    strlcpy(wifi_config.default_ap.password, WIFI_SETUP_AP_PASS, sizeof(wifi_config.default_ap.password));
    wifi_config.default_ap.channel = 1;
    wifi_config.always_use_ap_defaults = true; // Ignore any saved AP config - we want the captive portal to always be available and consistent
    wifi_config.enable_ap = true;

    // Share our HTTP server with wifi_cfg. API base path stays at the default
    // /api/wifi; Basic Auth stays off (the default).
    wifi_config.http.httpd = idf_httpd_get_handle();

    // ESP-IDF Network Provisioning over BLE
    wifi_config.prov_ble.device_name = "TiltBridge-{id}";
    wifi_config.prov_ble.security = WIFI_CFG_PROV_SECURITY_1;
    wifi_config.prov_ble.pop = "thorrak";
    // KEEP_ALL keeps the BT controller + BLE memory alive after the
    // provisioning manager tears down, so tilt_scanner can re-attach
    // via NimBLEDevice::init() without re-initialising the controller.
    wifi_config.prov_ble.memory_policy = WIFI_CFG_PROV_MEM_KEEP_ALL;
    // reset_on_failure defaults to false; set explicitly so a wrong-password
    // loop clears stored creds after max_failed_attempts and accepts a fresh
    // attempt without rebooting.
    wifi_config.prov_ble.reset_on_failure = true;
    wifi_config.prov_ble.max_failed_attempts = 3;

    // Initialize WiFi Config
    esp_err_t err = wifi_cfg_init(&wifi_config);
    if (err != ESP_OK) {
        Log.error("Failed to initialize WiFi Config: %d\r\n", err);
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    }

    // Push the config-file mDNS name into wifi_cfg's variable store now, before
    // provisioning can start, so the captive portal wizard shows the current
    // name. The var store is wiped by wifi_cfg_factory_reset() (disconnectWiFi)
    // and would otherwise fall back to the "tiltbridge" default until we connect.
    wifi_cfg_set_var("mdns_name", config.mdnsID);

    // Wait for connection (5 minute timeout)
    err = wifi_cfg_wait_connected(5 * 60 * 1000);
    if (err != ESP_OK) {
        Log.error("WiFi connection timeout. Restarting device.\r\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    }

    // wifi_cfg handles its own provisioning teardown after the configured delay
    // (stop_provisioning_on_connect + provisioning_teardown_delay_ms)

    // Sync mDNS name FROM config TO wifi_cfg (config file is the source of truth).
    // The on_var_changed callback handles the reverse direction for real-time changes.
    wifi_cfg_set_var("mdns_name", config.mdnsID);

    initMDNS();
}

// Check WiFi connectivity and trigger reconnect if needed.
// Called from the main loop to ensure the manager's auto-reconnect is engaged.
void reconnectWiFi() {
    if (!wifi_cfg_is_connected()) {
        wifi_cfg_connect(NULL);
    }
}

bool is_wifi_connected() {
    return wifi_cfg_is_connected();
}

bool get_local_ip(char* ip_str, size_t len) {
    // Primary: use esp_wifi_config's status API
    wifi_status_t status;
    if (wifi_cfg_get_status(&status) == ESP_OK && strlen(status.ip) > 0) {
        strlcpy(ip_str, status.ip, len);
        return true;
    }

    // Fallback: use ESP-IDF netif API directly
    // This handles edge cases where wifi_cfg status might not be updated yet
    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK && ip_info.ip.addr != 0) {
        snprintf(ip_str, len, IPSTR, IP2STR(&ip_info.ip));
        return true;
    }

    ip_str[0] = '\0';
    return false;
}
