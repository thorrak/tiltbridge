//
// Created by John Beeler on 6/4/18.
//

#include "wifi_setup.h"

// Since we can't use double reset detection or the "boot" button, we need
// to leverage the touchscreen to trigger the WiFi reset on TFT builds
#ifdef LCD_TFT
// #include <XPT2046_Touchscreen.h>
// XPT2046_Touchscreen ts(TS_CS);
#endif

bool shouldSaveConfig = false;
uint64_t wifi_reset_pressed_at = 0;
uint64_t board_reset_pressed_at = 0;

void saveParamsCallback()
{
    // Callback notifying us of the need to save config
    shouldSaveConfig = true;
}

void apCallback(WiFiManager *myWiFiManager)
{
    // Callback to display the WiFi LCD notification
    Log.verbose(F("Entered config mode: SSID: %s, IP: %s" CR), myWiFiManager->getConfigPortalSSID().c_str(), WiFi.softAPIP().toString().c_str());
    lcd.display_wifi_connect_screen(myWiFiManager->getConfigPortalSSID().c_str(), WIFI_SETUP_AP_PASS);
}

void disconnect_from_wifi_and_restart()
{
    WiFi.mode(WIFI_AP_STA);
    WiFi.persistent(true);
    WiFi.disconnect(true, true);
    WiFi.persistent(false);
    vTaskDelay(1000);
    ESP.restart();
}

void mdnsreset()
{
    tilt_scanner.wait_until_scan_complete(); // Wait for scans to complete (we don't want any tasks running in the background)
    http_server.name_reset_requested = false;
    ESP.restart();  // TODO:  This no longer works as designed
}

void init_wifi()
{
    WiFiManager wm;
#if ARDUINO_LOG_LEVEL == 6
    wm.setDebugOutput(true); // Use debug if we are at max log level
#else
    wm.setDebugOutput(false); // In case we have a serial connection
#endif

    // The main purpose of this is to set a boolean value which will allow us to know we
    // just saved a new configuration (as opposed to rebooting normally)
    wm.setSaveParamsCallback(saveParamsCallback);
    wm.setAPCallback(apCallback);
    wm.setConfigPortalTimeout(5 * 60); // Setting to 5 mins

    wm.setCleanConnect(true);
    wm.setCustomHeadElement("<style type=\"text/css\">body {background-image: url(\"data:image/svg+xml;utf8,%3Csvg%20id%3D%22Layer_1%22%20data-name%3D%22Layer%201%22%20xmlns%3D%22http%3A%2F%2Fwww.w3.org%2F2000%2Fsvg%22%20viewBox%3D%220%200%20231.62%20169.18%22%20xmlns%3Axlink%3D%22http%3A%2F%2Fwww.w3.org%2F1999%2Fxlink%22%20width%3D%2281.067%22%20height%3D%2259.213%22%3E%3Cdefs%3E%3Cstyle%3E.cls-1%7Bfill%3A%23f7f8fa%7D%3C%2Fstyle%3E%3C%2Fdefs%3E%3Cg%20id%3D%22tb_logo%22%3E%3Cg%3E%3Cpath%20class%3D%22cls-1%22%20d%3D%22M31.77%2C13.6A29.88%2C29.88%2C0%2C0%2C1%2C74%2C13.6a2.42%2C2.42%2C0%2C0%2C0%2C3.42-3.43%2C34.73%2C34.73%2C0%2C0%2C0-49.11%2C0%2C2.42%2C2.42%2C0%2C0%2C0%2C3.42%2C3.43Z%22%2F%3E%3Cpath%20class%3D%22cls-1%22%20d%3D%22M67.93%2C22.13A2.42%2C2.42%2C0%2C0%2C0%2C69.64%2C18a23.65%2C23.65%2C0%2C0%2C0-33.47%2C0%2C2.42%2C2.42%2C0%2C0%2C0%2C3.42%2C3.42%2C18.84%2C18.84%2C0%2C0%2C1%2C26.63%2C0A2.42%2C2.42%2C0%2C0%2C0%2C67.93%2C22.13Z%22%2F%3E%3Cpath%20class%3D%22cls-1%22%20d%3D%22M43.91%2C25.73a2.42%2C2.42%2C0%2C0%2C0%2C3.42%2C3.43%2C7.88%2C7.88%2C0%2C0%2C1%2C11.15%2C0%2C2.43%2C2.43%2C0%2C0%2C0%2C3.43-3.43A12.75%2C12.75%2C0%2C0%2C0%2C43.91%2C25.73Z%22%2F%3E%3C%2Fg%3E%3Cpath%20class%3D%22cls-1%22%20d%3D%22M103.39%2C61.76H91.7C80%2C61.76%2C70.34%2C57.24%2C63%2C48.32a45.48%2C45.48%2C0%2C0%2C1-7.38-12.7%2C3.85%2C3.85%2C0%2C0%2C1-5.36%2C0A45.31%2C45.31%2C0%2C0%2C1%2C43%2C48.19%2C35.71%2C35.71%2C0%2C0%2C1%2C14.12%2C61.76H2.42a2.42%2C2.42%2C0%2C0%2C0%2C0%2C4.84h101a2.42%2C2.42%2C0%2C0%2C0%2C0-4.84Zm-52.9%2C0H34.08A42.36%2C42.36%2C0%2C0%2C0%2C46.7%2C51.26%2C52%2C52%2C0%2C0%2C0%2C50.49%2C46Zm4.84%2C0V46a50.35%2C50.35%2C0%2C0%2C0%2C3.78%2C5.3%2C42.49%2C42.49%2C0%2C0%2C0%2C12.62%2C10.5Z%22%2F%3E%3Ccircle%20class%3D%22cls-1%22%20cx%3D%2252.91%22%20cy%3D%2232.86%22%20r%3D%222.81%22%2F%3E%3Cg%3E%3Cpath%20class%3D%22cls-1%22%20d%3D%22M22.39%2C77.09A2.44%2C2.44%2C0%2C0%2C1%2C20%2C74.65V64.52a2.45%2C2.45%2C0%2C0%2C1%2C4.89%2C0V74.65A2.45%2C2.45%2C0%2C0%2C1%2C22.39%2C77.09Z%22%2F%3E%3Cpath%20class%3D%22cls-1%22%20d%3D%22M83.42%2C77.09A2.45%2C2.45%2C0%2C0%2C1%2C81%2C74.65V64.52a2.45%2C2.45%2C0%2C0%2C1%2C4.89%2C0V74.65A2.45%2C2.45%2C0%2C0%2C1%2C83.42%2C77.09Z%22%2F%3E%3C%2Fg%3E%3C%2Fg%3E%3Cuse%20xlink%3Ahref%3D%22%23tb_logo%22%20x%3D%220%22%20y%3D%220%22%2F%3E%3Cuse%20xlink%3Ahref%3D%22%23tb_logo%22%20x%3D%22115.81%22%20y%3D%220%22%2F%3E%3Cuse%20xlink%3Ahref%3D%22%23tb_logo%22%20x%3D%22-57.905%22%20y%3D%2284.59%22%2F%3E%3Cuse%20xlink%3Ahref%3D%22%23tb_logo%22%20x%3D%2257.905%22%20y%3D%2284.59%22%2F%3E%3Cuse%20xlink%3Ahref%3D%22%23tb_logo%22%20x%3D%22173.715%22%20y%3D%2284.59%22%2F%3E%3C%2Fsvg%3E\")}</style>");

    // The third parameter we're passing here (mdns_id.c_str()) is the default name that will appear on the form.
    // It's nice, but it means the user gets no actual prompt for what they're entering.
    char mdns_id[31];
    strcpy(mdns_id, config.mdnsID);
    WiFiManagerParameter custom_mdns_name("mdns", "Device (mDNS) Name", mdns_id, 20);
    wm.addParameter(&custom_mdns_name);

    if (!wm.autoConnect(WIFI_SETUP_AP_NAME, WIFI_SETUP_AP_PASS))
    {
        Log.warning(F("Failed to connect and/or hit timeout. Restarting" CR));
        ESP.restart();
    }
    else
    {
        // We finished with portal (We were configured)
        // WiFi.softAPdisconnect(true);
        // WiFi.mode(WIFI_STA);
    }

    if (shouldSaveConfig)
    {
        // If we connected, then let's save the mDNS name
        LCBUrl url;
        // If the mDNS name is valid, save it.
        if (url.isValidHostName(custom_mdns_name.getValue()))
        {
            strlcpy(config.mdnsID, custom_mdns_name.getValue(), 31);
            strcpy(mdns_id, config.mdnsID);
        }
        else
        {
            // If the mDNS name is invalid, reset the WiFi configuration and restart
            // the ESP8266
            // TODO - add an LCD error message here maybe
            disconnect_from_wifi_and_restart();
        }
        saveConfig();
    }
    WiFi.setHostname(config.mdnsID);

    if (!MDNS.begin(config.mdnsID))
    {
        Log.error(F("Error setting up MDNS responder." CR));
    }

    MDNS.addService("http", "tcp", WEBPORT);       // technically we should wait on this, but I'm impatient.
    MDNS.addService("tiltbridge", "tcp", WEBPORT); // for lookups
#if DOTELNET == true
    MDNS.addService("telnet", "tcp", TELNETPORT);
#endif

    // Display a screen so the user can see how to access the Tiltbridge
    char mdns_url[50] = "http://";
    strncat(mdns_url, mdns_id, 31);
    strcat(mdns_url, ".local/");

    char ip_address_url[25] = "http://";
    char ip[16];
    sprintf(ip, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
    strncat(ip_address_url, ip, 16);
    strcat(ip_address_url, "/");

    lcd.display_wifi_success_screen(mdns_url, ip_address_url);
}

#ifndef LCD_TFT
// Use the "boot" button present on most of the OLED boards to reset the WiFi configuration allowing for easy
// transportation between networks
void IRAM_ATTR wifi_reset_pressed()
{
    // When the reset button is pressed, just log the time & get back to work
    wifi_reset_pressed_at = xTaskGetTickCount();
}

void IRAM_ATTR board_reset_pressed()
{
    // When the reset button is pressed, just log the time & get back to work
    board_reset_pressed_at = xTaskGetTickCount();
}

void initWiFiResetButton()
{
    pinMode(WIFI_RESET_BUTTON_GPIO, INPUT_PULLUP);
    attachInterrupt(WIFI_RESET_BUTTON_GPIO, wifi_reset_pressed, RISING);
}

void initBoardResetButton()
{
    pinMode(BOARD_RESET_BUTTON_GPIO, INPUT_PULLUP);
    attachInterrupt(BOARD_RESET_BUTTON_GPIO, board_reset_pressed, RISING);
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

void initBoardResetButton()
{
    // This is noop for LCD_TFT as we're using the touchscreen instead
}

#endif

void handle_wifi_reset_presses()
{
    uint64_t initial_press_at = 0;

#ifdef LCD_TFT
    // while (ts->touched()) // Block while the screen is pressed until the user releases
    //     wifi_reset_pressed_at = xTaskGetTickCount();
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
            // if (ts.touched() || wifi_reset_pressed_at != initial_press_at)
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

        delay(WIFI_RESET_DOUBLE_PRESS_TIME); // Block while we let the user press a second time

        if(wifi_reset_pressed_at != initial_press_at) {
            // The user pushed the button a second time & caused a second interrupt. Process the reset.
            disconnect_from_wifi_and_restart();
        }
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
