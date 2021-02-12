#include "wifi_setup.h"

Ticker wifiCheck;

bool shouldSaveConfig = false;

void saveParamsCallback() {
    // Callback notifying us of the need to save config
    shouldSaveConfig = true;
}

void apCallback(WiFiManager *myWiFiManager) {
    // Callback to display the WiFi LCD notification and set bandwidth
    Log.verbose(F("Entered config mode: SSID: %s, IP: %s" CR), myWiFiManager->getConfigPortalSSID().c_str(), WiFi.softAPIP().toString().c_str());
    lcd.display_wifi_connect_screen(myWiFiManager->getConfigPortalSSID().c_str(), WIFI_SETUP_AP_PASS);
    esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);  // Set the bandwidth of ESP32 interface 
}

void disconnectWiFi() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.persistent(true);
    WiFi.disconnect(true, true);
    WiFi.persistent(false);
    vTaskDelay(1000);
    ESP.restart();
}

void mdnsReset() {
    tilt_scanner.wait_until_scan_complete(); // Wait for scans to complete
    http_server.name_reset_requested = false;
    MDNS.end();
    if (!MDNS.begin(config.mdnsID)) {
        Log.error(F("Error resetting MDNS responder."));
        ESP.restart();
    } else {
        Log.notice(F("mDNS responder restarted, hostname: %s.local." CR), WiFi.getHostname());
        MDNS.addService("http", "tcp", WEBPORT);
        MDNS.addService("tiltbridge", "tcp", WEBPORT);
#if DOTELNET == true
        MDNS.addService("telnet", "tcp", TELNETPORT);
#endif
    }
}

void initWiFi() {
    WiFiManager wm;
#if ARDUINO_LOG_LEVEL == 6
    wm.setDebugOutput(true); // Use debug if we are at max log level
#else
    wm.setDebugOutput(false);
#endif

    wm.setSaveParamsCallback(saveParamsCallback);   // Signals settings change
    wm.setAPCallback(apCallback);                   // Set up when portal fires
    wm.setConfigPortalTimeout(5 * 60);              // Timeout portal in 5 mins

    wm.setHostname(config.mdnsID);  // Allow DHCP to get proper name
    wm.setWiFiAutoReconnect(true);  // Enable auto reconnect (should remove need for reconnectWiFi())
    wm.setWiFiAPChannel(1);         // Pick the most common channel, safe for all countries
    wm.setCleanConnect(true);       // Always disconnect before connecting
    wm.setCountry("US");            // US country code is most restrictive, use for all countries

    wm.setCustomHeadElement("<style type=\"text/css\">body {background-image: url(\"data:image/svg+xml;utf8,%3Csvg%20id%3D%22Layer_1%22%20data-name%3D%22Layer%201%22%20xmlns%3D%22http%3A%2F%2Fwww.w3.org%2F2000%2Fsvg%22%20viewBox%3D%220%200%20231.62%20169.18%22%20xmlns%3Axlink%3D%22http%3A%2F%2Fwww.w3.org%2F1999%2Fxlink%22%20width%3D%2281.067%22%20height%3D%2259.213%22%3E%3Cdefs%3E%3Cstyle%3E.cls-1%7Bfill%3A%23f7f8fa%7D%3C%2Fstyle%3E%3C%2Fdefs%3E%3Cg%20id%3D%22tb_logo%22%3E%3Cg%3E%3Cpath%20class%3D%22cls-1%22%20d%3D%22M31.77%2C13.6A29.88%2C29.88%2C0%2C0%2C1%2C74%2C13.6a2.42%2C2.42%2C0%2C0%2C0%2C3.42-3.43%2C34.73%2C34.73%2C0%2C0%2C0-49.11%2C0%2C2.42%2C2.42%2C0%2C0%2C0%2C3.42%2C3.43Z%22%2F%3E%3Cpath%20class%3D%22cls-1%22%20d%3D%22M67.93%2C22.13A2.42%2C2.42%2C0%2C0%2C0%2C69.64%2C18a23.65%2C23.65%2C0%2C0%2C0-33.47%2C0%2C2.42%2C2.42%2C0%2C0%2C0%2C3.42%2C3.42%2C18.84%2C18.84%2C0%2C0%2C1%2C26.63%2C0A2.42%2C2.42%2C0%2C0%2C0%2C67.93%2C22.13Z%22%2F%3E%3Cpath%20class%3D%22cls-1%22%20d%3D%22M43.91%2C25.73a2.42%2C2.42%2C0%2C0%2C0%2C3.42%2C3.43%2C7.88%2C7.88%2C0%2C0%2C1%2C11.15%2C0%2C2.43%2C2.43%2C0%2C0%2C0%2C3.43-3.43A12.75%2C12.75%2C0%2C0%2C0%2C43.91%2C25.73Z%22%2F%3E%3C%2Fg%3E%3Cpath%20class%3D%22cls-1%22%20d%3D%22M103.39%2C61.76H91.7C80%2C61.76%2C70.34%2C57.24%2C63%2C48.32a45.48%2C45.48%2C0%2C0%2C1-7.38-12.7%2C3.85%2C3.85%2C0%2C0%2C1-5.36%2C0A45.31%2C45.31%2C0%2C0%2C1%2C43%2C48.19%2C35.71%2C35.71%2C0%2C0%2C1%2C14.12%2C61.76H2.42a2.42%2C2.42%2C0%2C0%2C0%2C0%2C4.84h101a2.42%2C2.42%2C0%2C0%2C0%2C0-4.84Zm-52.9%2C0H34.08A42.36%2C42.36%2C0%2C0%2C0%2C46.7%2C51.26%2C52%2C52%2C0%2C0%2C0%2C50.49%2C46Zm4.84%2C0V46a50.35%2C50.35%2C0%2C0%2C0%2C3.78%2C5.3%2C42.49%2C42.49%2C0%2C0%2C0%2C12.62%2C10.5Z%22%2F%3E%3Ccircle%20class%3D%22cls-1%22%20cx%3D%2252.91%22%20cy%3D%2232.86%22%20r%3D%222.81%22%2F%3E%3Cg%3E%3Cpath%20class%3D%22cls-1%22%20d%3D%22M22.39%2C77.09A2.44%2C2.44%2C0%2C0%2C1%2C20%2C74.65V64.52a2.45%2C2.45%2C0%2C0%2C1%2C4.89%2C0V74.65A2.45%2C2.45%2C0%2C0%2C1%2C22.39%2C77.09Z%22%2F%3E%3Cpath%20class%3D%22cls-1%22%20d%3D%22M83.42%2C77.09A2.45%2C2.45%2C0%2C0%2C1%2C81%2C74.65V64.52a2.45%2C2.45%2C0%2C0%2C1%2C4.89%2C0V74.65A2.45%2C2.45%2C0%2C0%2C1%2C83.42%2C77.09Z%22%2F%3E%3C%2Fg%3E%3C%2Fg%3E%3Cuse%20xlink%3Ahref%3D%22%23tb_logo%22%20x%3D%220%22%20y%3D%220%22%2F%3E%3Cuse%20xlink%3Ahref%3D%22%23tb_logo%22%20x%3D%22115.81%22%20y%3D%220%22%2F%3E%3Cuse%20xlink%3Ahref%3D%22%23tb_logo%22%20x%3D%22-57.905%22%20y%3D%2284.59%22%2F%3E%3Cuse%20xlink%3Ahref%3D%22%23tb_logo%22%20x%3D%2257.905%22%20y%3D%2284.59%22%2F%3E%3Cuse%20xlink%3Ahref%3D%22%23tb_logo%22%20x%3D%22173.715%22%20y%3D%2284.59%22%2F%3E%3C%2Fsvg%3E\")}</style>");

    // config.mdnsID is the default name that will appear on the form
    WiFiManagerParameter custom_mdns_name("mdns", "Device (mDNS) Name", config.mdnsID, 20);
    wm.addParameter(&custom_mdns_name);

    if (!wm.autoConnect(WIFI_SETUP_AP_NAME, WIFI_SETUP_AP_PASS)) {
        Log.warning(F("Failed to connect and/or hit timeout. Restarting." CR));
        ESP.restart();
    } else {
        // We finished with portal (We were configured)
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
    }

    if (shouldSaveConfig) {
        // If we connected, then let's save the mDNS name
        LCBUrl url;
        if (strcmp(custom_mdns_name.getValue(), config.mdnsID) != 0) {
            // If the mDNS name is valid, save it.
            if (url.isValidHostName(custom_mdns_name.getValue())) {
                strlcpy(config.mdnsID, custom_mdns_name.getValue(), 31);
                saveConfig();
            } else {
                // If the mDNS name is invalid, reset the WiFi configuration and restart the ESP8266
                disconnectWiFi();
            }
        }
        // Doing this to reset the DHCP name, the portal should never pop
        // Additionally, there is a bug where the HTTP server doesn't spin up after the AP shuts down. Not sure where
        // that issue is, but this solves it.
        ESP.restart();
    }

    if (!MDNS.begin(config.mdnsID)) {
        Log.error(F("Error setting up MDNS responder." CR));
    }

    MDNS.addService("http", "tcp", WEBPORT);       // technically we should wait on this, but I'm impatient.
    MDNS.addService("tiltbridge", "tcp", WEBPORT); // for lookups
#if DOTELNET == true
    MDNS.addService("telnet", "tcp", TELNETPORT);
#endif

    // Display a screen so the user can see how to access the Tiltbridge
    char mdns_url[50] = "http://";
    strncat(mdns_url, config.mdnsID, 31);
    strcat(mdns_url, ".local/");

    char ip_address_url[25] = "http://";
    char ip[16];
    sprintf(ip, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
    strncat(ip_address_url, ip, 16);
    strcat(ip_address_url, "/");

    lcd.display_wifi_success_screen(mdns_url, ip_address_url);
    wifiCheck.attach(1, reconnectWiFi); // Check on WiFi
}

void reconnectWiFi() {
    if (WiFiClass::status() != WL_CONNECTED) {
        // WiFi is down - Reconnect
        lcd.display_wifi_disconnected_screen();
        WiFi.begin();

        delay(1000); // Ensuring the "disconnected" screen appears for at least one second

        int WLcount = 0;
        while (WiFiClass::status() != WL_CONNECTED && WLcount < 190) {
            delay(100);
            printDot(true);
            ++WLcount;
        }

        if (WiFiClass::status() != WL_CONNECTED) {
            // We failed to reconnect.
            lcd.display_wifi_reconnect_failed();
        } else {
            // We reconnected successfully
            lcd.display_logo();
        }
    }
}
