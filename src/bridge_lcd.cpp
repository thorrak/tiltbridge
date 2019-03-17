//
// Created by John Beeler on 5/12/18.
//

#include "bridge_lcd.h"
#include "img/fermentrack_logo.h"

bridge_lcd lcd;


#include <Wire.h>
#ifdef LCD_SSD1306
#include <SSD1306.h>
#endif




bridge_lcd::bridge_lcd() {
    next_screen_at = 0;
    on_screen = 0;  // Initialize to 0 (AKA screen_tilt)
    tilt_on_page = 0;
    tilt_pages_in_run = 0;

}  // bridge_lcd




void bridge_lcd::display_logo() {
    // XBM files are C source bitmap arrays, and can be created in GIMP (and then read/imported using text editors)
#ifdef LCD_SSD1306
    clear();
    oled_display->drawXbm((128-fermentrack_logo_width)/2, (64-fermentrack_logo_height)/2, fermentrack_logo_width, fermentrack_logo_height, fermentrack_logo_bits);
    display();
#endif
}


void bridge_lcd::check_screen() {
    if(next_screen_at < xTaskGetTickCount()) {
        next_screen_at = display_next() * 1000 + xTaskGetTickCount();
    }
}

// display_next returns the number of seconds to "hold" on this screen
uint8_t bridge_lcd::display_next() {
    uint8_t active_tilts = 0;

    if(on_screen==SCREEN_TILT) {
        if(tilt_pages_in_run==0) {
            // This is the first time we're displaying a tilt screen in this round. Figure out how many pages we need
            for(uint8_t i = 0;i<TILT_COLORS;i++) {
                if (tilt_scanner.tilt(i)->is_loaded())
                    active_tilts++;
            }

            // We'll always have at least one page, but we can have more
            tilt_pages_in_run = (active_tilts/TILTS_PER_PAGE) + 1;
            tilt_on_page = 0;
        }

        display_tilt_screen(tilt_on_page);

        tilt_on_page++;
        if(tilt_on_page >= tilt_pages_in_run) {
            tilt_pages_in_run = 0;  // We've displayed the last page
            tilt_on_page = 0;
            on_screen++;
        }

        return 10;  // Display this screen for 10 seconds

    } else if(on_screen==SCREEN_FERMENTRACK) {
        display_logo();
        on_screen++;
        return 5;  // This is currently a noop
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

    // Display the header row
    print_line("Color", "Gravity", 1);

    // Loop through each of the tilt colors cached by tilt_scanner, searching for active tilts
    for(uint8_t i = 0;i<TILT_COLORS;i++) {
        if(tilt_scanner.tilt(i)->is_loaded()) {
            active_tilts++;
            // This check has the added bonus of limiting the # of displayed tilts to TILTS_PER_PAGE
            if((active_tilts/TILTS_PER_PAGE)==screen_number) {
                print_tilt_to_line(tilt_scanner.tilt(i), displayed_tilts+2);
                displayed_tilts++;
            }
        }
    }

    // Toggle the actual display
    display();

}


void bridge_lcd::display_wifi_connect_screen(String ap_name, String ap_pass) {
    clear();
    //         "**********8888888888   **********8888888888"
    print_line("To configure, connect to", "", 1);
    print_line("this AP via WiFi:", "", 2);
    print_line("Name:", ap_name, 3);
    print_line("Pass: ", ap_pass, 4);
    display();
}

void bridge_lcd::display_wifi_fail_screen() {
         //    "**********8888888888 **********8888888888"
    clear();
    print_line("Failed to connect.", "", 1);
    print_line("Restart TiltBridge,", "", 2);
    print_line("connect to config AP", "", 3);
    print_line("and try again.", "", 4);
    display();
}

void bridge_lcd::display_wifi_success_screen(String mdns_url, String ip_address_url) {
    //    "**********8888888888 **********8888888888"
    clear();
    print_line("Access your TiltBridge at:", "", 1);
    print_line(mdns_url, "", 2);
    print_line(ip_address_url, "", 3);
    display();
}


void bridge_lcd::print_tilt_to_line(tiltHydrometer* tilt, uint8_t line) {
    char gravity[10];
    sprintf(gravity, "%.3f", double_t(tilt->gravity)/1000);
    print_line(tilt->color_name().c_str(), gravity, line);
}




bool bridge_lcd::i2c_device_at_address(byte address, int sda_pin, int scl_pin) {
    byte error;

    Wire.begin(sda_pin, scl_pin);
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)  // No error means that a device responded
        return true;
    else
        return false;
}



/////////// LCD Wrapper Functions

void bridge_lcd::init() {
#ifdef LCD_SSD1306

    // We're currently supporting three sets of hardware - The ESP32 "OLED" board, TTGO Boards, and the TiltBridge sleeve
    if(i2c_device_at_address(0x3c, 5, 4)) {
        // This is the ESP32 "OLED" board
        oled_display = new SSD1306(0x3c, 5, 4);
    } else if(i2c_device_at_address(0x3c, 21, 22)) {
        // This is the TiltBridge "sleeve"
        // Address, SDA, SCK
        oled_display = new SSD1306(0x3c, 21, 22);
    } else {
        // For the "TTGO" style OLED shields, you have to power a pin to run the backlight.
        pinMode(16,OUTPUT);
        digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
        delay(50);
        digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high
        if(i2c_device_at_address(0x3c, 4, 15)) {
            oled_display = new SSD1306(0x3c, 4, 15);
        } else {
            digitalWrite(16, LOW);    // We weren't able to find the TTGO board, so reset the pin
            oled_display = new SSD1306(0x3c, 21, 22);  // ... and just default to the "sleeve" configuration
        }
    }

    oled_display->init();
    oled_display->flipScreenVertically();
    oled_display->setFont(ArialMT_Plain_10);
#endif
}


void bridge_lcd::clear() {
#ifdef LCD_SSD1306
    oled_display->clear();
    oled_display->setFont(SSD1306_FONT);
#endif
}

void bridge_lcd::display() {
#ifdef LCD_SSD1306
    oled_display->display();
#endif
}


void bridge_lcd::print_line(String left_text, String right_text, uint8_t line) {
#ifdef LCD_SSD1306
    int16_t starting_pixel_row = 0;

    starting_pixel_row = (SSD_LINE_CLEARANCE + SSD1306_FONT_HEIGHT) * (line-1) + SSD_LINE_CLEARANCE;

    // The coordinates define the left starting point of the text
    oled_display->setTextAlignment(TEXT_ALIGN_LEFT);
    oled_display->drawString(0, starting_pixel_row, left_text);

    oled_display->setTextAlignment(TEXT_ALIGN_RIGHT);
    oled_display->drawString(128, starting_pixel_row, right_text);
#endif
}

