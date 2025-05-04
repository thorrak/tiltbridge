#include <ArduinoLog.h>
#include <WiFi.h>

#ifdef LCD_SSD1306
#include <Wire.h>
#endif

#include "jsonconfig.h"
#include "tilt/tiltScanner.h"
#include "bridge_lcd.h"

bridge_lcd lcd;

#if defined(LCD_SSD1306) || defined(LCD_TFT_ESPI)
#include "img/oled_logo.h" // Small logo
#elif defined(LCD_TFT)
#include "img/tft_logo.h" // Large logo
#endif


bool onResetScreen = false;

bridge_lcd::bridge_lcd() {
#if HAVE_LCD
    next_screen_at = 0;
    on_screen = 0; // Initialize to 0 (AKA screen_tilt)
    tilt_on_page = 0;
    tilt_pages_in_run = 0;
#endif
}

////////////////////////////////////////////////////////////
// Public Methods
////////////////////////////////////////////////////////////

#if HAVE_LCD

void bridge_lcd::display_logo(bool fromReset) {
    clear();

    if (fromReset) {
        // Need this if we got here from reset timeout
        on_screen = SCREEN_TILT;
        onResetScreen = false;
        next_screen_at = millis() + 2000;
    }

    display_logo_internal();
}

void bridge_lcd::display_wifi_connect_screen(const char *ap_name, const char *ap_pass) {
    // Displayed when the user first plugs in an unconfigured device
    clear();
    print_line("To configure, connect to", 1);
    print_line("this AP via WiFi:", 2);
    print_line("Name:", ap_name, 3);
    print_line("Pass: ", ap_pass, 4);

#ifdef LCD_TFT
// TODO - See if the next line can be removed without issue
    // tft->setFreeFont(FF_NORMAL);
    print_line("NOTE - If this appears upside-down,", 8);
    print_line("this can be corrected via a setting in the", 9);
    print_line("settings portal after connecting to WiFi", 10);
#endif

    display();
}

void bridge_lcd::display_wifi_success_screen(const char *mdns_url, const char *ip_address_url) {
    // Displayed at startup when the device is configured to connect to WiFi
    clear();

    print_line("Access this device at:", 1);
    print_line(mdns_url, 2);
    print_line(ip_address_url, 3);
    display();
}

void bridge_lcd::display_wifi_reset_screen() {
    // When the user presses the "boot" switch, this screen appears. If the
    // user presses the boot button a second time while this screen is
    // displayed, WiFi settings are cleared and the device will return
    // to displaying the configuration AP at startup
    clear();
    onResetScreen = true;

#if defined(LCD_SSD1306) || defined(LCD_TFT_ESPI)
    print_line("Press the button again to", 1);
    print_line("disable autoconnection", 2);
    print_line("and start the WiFi ", 3);
    print_line("configuration AP.", 4);
    display();
#elif defined(LCD_TFT)
    print_line("Tap the screen again to", 1);
    print_line("delete any saved WiFi", 2);
    print_line("credentials and restart", 3);
    print_line("the WiFi configuration AP", 4);
#endif
}

void bridge_lcd::display_ota_update_screen()
{
    // When the user presses the "boot" switch, this screen appears. If the
    // user presses the boot button a second time while this screen is
    // displayed, WiFi settings are cleared and the TiltBridge will return
    // to displaying the configuration AP at startup
#ifndef DISABLE_OTA_UPDATES
    clear();
    print_line("The TiltBridge firmware is", "", 1);
    print_line("being updated. Please do", "", 2);
    print_line("not power down or reset", "", 3);
    print_line("your TiltBridge.", "", 4);
    display();
#endif
}

void bridge_lcd::check_screen() {
    if (!onResetScreen && !displaying_ota_update_screen && next_screen_at < millis()) {
        next_screen_at = display_next() * 1000 + millis();
    }
}

#else // HAVE_LCD

// No-op implementation.
void bridge_lcd::init() {}
void bridge_lcd::reinit() {}
void bridge_lcd::display_logo(bool fromReset) {}
void bridge_lcd::checkTouch() {}

void bridge_lcd::display_wifi_connect_screen(const char *ap_name, const char *ap_pass) {}
void bridge_lcd::display_wifi_success_screen(const char *mdns_url, const char *ip_address_url) {}
void bridge_lcd::display_wifi_reset_screen() {}
void bridge_lcd::display_ota_update_screen() {}

void bridge_lcd::display_wifi_disconnected_screen() {}
void bridge_lcd::display_wifi_reconnect_failed() {}

void bridge_lcd::print_line(const char *left_text, const char *right_text, uint8_t line) {}
void bridge_lcd::print_line(const char *left_text, const char *middle_text, const char *right_text, uint8_t line) {}
void bridge_lcd::print_line(const char *left_text, const char *middle_text, const char *right_text, uint8_t line, bool add_gutter) {}

void bridge_lcd::check_screen() {}
void bridge_lcd::clear() {}

#endif // HAVE_LCD

////////////////////////////////////////////////////////////
// Private Methods
////////////////////////////////////////////////////////////

#if HAVE_LCD

uint8_t bridge_lcd::display_next() {
    // Returns the number of seconds to "hold" on this screen
    uint8_t active_tilts = 0;

    if (WiFi.status() == WL_CONNECTED) {
        displaying_wifi_dc_screen = false;
    }

    if (displaying_wifi_dc_screen) {
        // If we're displaying the wifi dc screen, stay on it. 
        return 1;
    }

    if (on_screen == SCREEN_TILT) {
        if (tilt_pages_in_run == 0) {
            // This is the first time we're displaying a tilt screen in this round. Figure out how many pages we need
            active_tilts = tilt_scanner.tilt_count();

            // We'll always have at least one page, but we can have more
            tilt_pages_in_run = (active_tilts / TILTS_PER_PAGE) + 1;
            tilt_on_page = 0;
        }

        display_tilt_screen(tilt_on_page);

        tilt_on_page++;
        if (tilt_on_page >= tilt_pages_in_run) {
            tilt_pages_in_run = 0; // We've displayed the last page
            tilt_on_page = 0;
            on_screen++;
        }

        return TILT_TIME; // Display this screen for 10 seconds
    } else if (on_screen == SCREEN_LOGO) {
        display_logo();
        on_screen++;
        return LOGO_TIME; // This is currently a noop
    } else {
        on_screen = SCREEN_TILT;
        return 0; // Immediately move on to the next screen
    }
}

void bridge_lcd::display_tilt_screen(uint8_t screen_number) {
    uint8_t active_tilts = 0;
    uint8_t displayed_tilts = 0;

    // Clear out the display before we start printing to it
    clear();

    // Display IP address on bottom row if using Lolin TFT
    uint8_t header_row = 1;
    uint8_t first_tilt_row_offset = 2;

#ifdef LCD_TFT
    // Display IP address or indicate if not connected
    if (WiFi.status() == WL_CONNECTED) {
        char ip[16];
        sprintf(ip, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
        print_line("IP Address:", ip, 11);
    } else {
        print_line("No WiFi Connection", "", 10);
    }
    header_row = 1;
    first_tilt_row_offset = header_row + 1;
#endif

    // Display the header row
    print_line("Color", "Temp", "Gravity", header_row, true);

    // Loop through each of the tilts cached by tilt_scanner
    tilt_scanner.drop_expired_tilts();
    for(tiltHydrometer & th : tilt_scanner.m_tilt_devices) {
            active_tilts++;
            // This check has the added bonus of limiting the # of displayed tilts to TILTS_PER_PAGE
            if ((active_tilts / TILTS_PER_PAGE) == screen_number) {
                print_tilt_to_line(&th, displayed_tilts + first_tilt_row_offset);
                displayed_tilts++;
            }
    }

    // Toggle the actual display
    display();
}

void bridge_lcd::display_wifi_disconnected_screen() {
    // If the user's WiFi disconnects for any reason, it can take up to
    // 20 seconds to reconnect. We'll print a message letting the user
    // know while we attempt to reconnect.
    clear();
    displaying_wifi_dc_screen = true;
    print_line("This device has lost", 1);
    print_line("connection to your WiFi.", 2);
    print_line("", 3);
    print_line("Attempting to reconnect...", 4);
    display();
}

void bridge_lcd::display_wifi_reconnect_failed() {
    // If the user's WiFi disconnects for any reason, it can take up to
    // 20 seconds to reconnect. We'll print a message letting the user
    // know while we attempt to reconnect.
    clear();
    displaying_wifi_dc_screen = true;
    print_line("This device was unable to", 1);
    print_line("reconnect to your WiFi.", 2);
    print_line("", 3);
    print_line("Restarting...", 4);
    display();
}

#endif // HAVE_LCD

void screenFlip() {
    lcd.check_screen();
}