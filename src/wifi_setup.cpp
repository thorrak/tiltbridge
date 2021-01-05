//
// Created by John Beeler on 6/4/18.
// Modified by Tim Pletcher on 31-Oct-2020.
//

#include "wifi_setup.h"

// Since we can't use double reset detection or the "boot" button, we need to leverage the touchscreen to trigger the
// WiFi reset on TFT builds
#ifdef LCD_TFT
#include <XPT2046_Touchscreen.h>
XPT2046_Touchscreen ts(TS_CS);
#endif

bool shouldSaveConfig = false;
uint64_t wifi_reset_pressed_at = 0;

//callback notifying us of the need to save config
// TODO - This can probably be eliminated
void saveConfigCallback()
{
    //    Serial.println("Should save config");
    shouldSaveConfig = true;
}

// callback to display the WiFi LCD notification
void configModeCallback(AsyncWiFiManager *myWiFiManager)
{
    Log.verbose(F("Entered config mode: SSID: %s, IP: %s"), myWiFiManager->getConfigPortalSSID().c_str(), WiFi.softAPIP().toString().c_str());
    // Assuming WIFI_SETUP_AP_PASS here.
    lcd.display_wifi_connect_screen(myWiFiManager->getConfigPortalSSID().c_str(), WIFI_SETUP_AP_PASS);
}

// Not sure if this is sufficient to test for validity
bool isValidmDNSName(const char *mdns_name)
{
    if (strlen(mdns_name) > 31 || strlen(mdns_name) < 8 || mdns_name[0] == '-' || mdns_name[strlen(mdns_name) - 1] == '-')
        return false;
    for (int i = 0; i < strlen(mdns_name); i++)
    {
        // For now, we're just checking that every character in the string is alphanumeric. May need to add more validation here.
        if (!isalnum(mdns_name[i]) && mdns_name[i] != '-')
            return false;
    }
    return true;
}

void disconnect_from_wifi_and_restart()
{
    WiFi.disconnect(true, true);
    WiFi.begin("0", "0");
    delay(1000);
    ESP.restart();
}

void init_wifi()
{
    AsyncWiFiManager wifiManager;      //Local initialization. Once its business is done, there is no need to keep it around
    wifiManager.setDebugOutput(false); // In case we have a serial connection

    // The main purpose of this is to set a boolean value which will allow us to know we
    // just saved a new configuration (as opposed to rebooting normally)
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.setAPCallback(configModeCallback);
    wifiManager.setConfigPortalTimeout(5 * 60); // Setting to 5 mins
    wifiManager.setConnectRetries(3);
    wifiManager.setCleanConnect(true);

    // The third parameter we're passing here (mdns_id.c_str()) is the default name that will appear on the form.
    // It's nice, but it means the user gets no actual prompt for what they're entering.
    char mdns_id[31];
    strcpy(mdns_id, config.mdnsID);
    AsyncWiFiManagerParameter custom_mdns_name("mdns", "Device (mDNS) Name", mdns_id, 20);
    wifiManager.addParameter(&custom_mdns_name);

    if (wifiManager.autoConnect(WIFI_SETUP_AP_NAME, WIFI_SETUP_AP_PASS))
    {
        // TODO - Determine if we can merge shouldSaveConfig in here
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
    }
    else
    {
        // If we haven't successfully connected to WiFi, just restart & continue to project the configuration AP.
        // Alternatively, we can hang here.
        ESP.restart();
    }

    // Alright. We're theoretically connected here.
    // If we connected, then let's save the mDNS name
    if (shouldSaveConfig)
    {
        // If the mDNS name is valid, save it.
        if (isValidmDNSName(custom_mdns_name.getValue()))
        {
            strlcpy(config.mdnsID, custom_mdns_name.getValue(), 31);
            strcpy(mdns_id, config.mdnsID);
        }
        else
        {
            // If the mDNS name is invalid, reset the WiFi configuration and restart the ESP8266
            // TODO - add an LCD error message here maybe
            disconnect_from_wifi_and_restart();
        }

        //        app_config.config["password"] = custom_password.getValue();
        saveConfig();
    }

    if (!MDNS.begin(mdns_id))
    {
        Log.error(F("Error setting up MDNS responder." CR));
    }

    MDNS.addService("http", "tcp", 80);       // technically we should wait on this, but I'm impatient.
    MDNS.addService("tiltbridge", "tcp", 80); // for lookups

    // Display a screen so the user can see how to access the Tiltbridge
    char mdns_url[50] = "http://";
    strncat(mdns_url, mdns_id, 31);
    strcat(mdns_url, ".local");

    char ip_address_url[25] = "http://";
    char ip[16];
    sprintf(ip, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
    strncat(ip_address_url, ip, 16);
    strcat(ip_address_url, "/");

    lcd.display_wifi_success_screen(mdns_url, ip_address_url);
    // In order to have the system register the mDNS name in DHCP table, it is necessary to flush config
    // and reinitialize Wifi connection. If this is not done, the DHCP hostname is always just registered
    // as espressif.  See: https://github.com/espressif/arduino-esp32/issues/2537
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname(mdns_id);
    WiFi.begin();
    delay(1000);
}

#ifndef LCD_TFT
// Use the "boot" button present on most of the OLED boards to reset the WiFi configuration allowing for easy
// transportation between networks
void IRAM_ATTR wifi_reset_pressed()
{
    // When the reset button is pressed, just log the time & get back to work
    wifi_reset_pressed_at = xTaskGetTickCount();
}

void initWiFiResetButton()
{
    pinMode(WIFI_RESET_BUTTON_GPIO, INPUT_PULLUP);
    attachInterrupt(WIFI_RESET_BUTTON_GPIO, wifi_reset_pressed, RISING);
}

//void disableWiFiResetButton() {
//    detachInterrupt(WIFI_RESET_BUTTON_GPIO);
//}

#else
// If we have LCD_TFT set, we need to build the functions as noop
void initWiFiResetButton()
{
    // This is noop for LCD_TFT as we're using the touchscreen instead
}

#endif

void handle_wifi_reset_presses()
{
    uint64_t initial_press_at = 0;

#ifdef LCD_TFT
    while (ts.touched()) // Block while the screen is pressed until the user releases
        wifi_reset_pressed_at = xTaskGetTickCount();
#endif

    if (wifi_reset_pressed_at > (xTaskGetTickCount() - WIFI_RESET_DOUBLE_PRESS_TIME) && wifi_reset_pressed_at > WIFI_RESET_DOUBLE_PRESS_TIME)
    {
        initial_press_at = wifi_reset_pressed_at; // Cache when the button was first pressed
        lcd.display_wifi_reset_screen();
        delay(100); // Give the user a moment to release the screen (doubles as debounce)

        for (TickType_t x = xTaskGetTickCount() + WIFI_RESET_DOUBLE_PRESS_TIME; xTaskGetTickCount() <= x;)
        {
            delay(1);

#ifdef LCD_TFT
            if (ts.touched() || wifi_reset_pressed_at != initial_press_at)
#else
            if (wifi_reset_pressed_at != initial_press_at)
#endif
            {
                // The user pushed the button a second time & caused a second interrupt. Process the reset.
                disconnect_from_wifi_and_restart();
            }
        }

        // Explicitly clear the screen
        lcd.clear();

        //        delay(WIFI_RESET_DOUBLE_PRESS_TIME); // Block while we let the user press a second time
        //
        //        if(wifi_reset_pressed_at != initial_press_at) {
        //            // The user pushed the button a second time & caused a second interrupt. Process the reset.
        //            disconnect_from_wifi_and_restart();
        //        }
    }
}

void reconnectIfDisconnected()
{
    if (WiFiClass::status() != WL_CONNECTED)
    {
        // WiFi is down - Reconnect
        lcd.display_wifi_disconnected_screen();
        WiFi.begin();

        delay(1000); // Ensuring the "disconnected" screen appears for at least one second

        int WLcount = 0;
        while (WiFiClass::status() != WL_CONNECTED && WLcount < 190)
        {
            delay(100);
            printDot(true);
            ++WLcount;
        }

        if (WiFiClass::status() != WL_CONNECTED)
        {
            // We failed to reconnect.
            lcd.display_wifi_reconnect_failed();
        }
        else
        {
            // We reconnected successfully
            lcd.display_logo();
        }
    }
}
