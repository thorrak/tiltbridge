//
// Created by John Beeler on 5/12/18.
// Modified by Tim Pletcher on 31-Oct-2020.
//

#include "bridge_lcd.h"
#include "jsonConfigHandler.h"
#include <WiFi.h>

bridge_lcd lcd;


#ifdef LCD_SSD1306
#include <Wire.h>
#include "img/fermentrack_logo.h"  // We're only using this style of logo for the OLED variant
#endif

#ifdef LCD_TFT
#include "img/tiltbridge_logo_tft.h"  // The (large) TiltBridge logo
#endif

#ifdef LCD_TFT_ESPI
#include "img/fermentrack_logo.h"  // We're only using this style of logo for the OLED variant
#endif


bridge_lcd::bridge_lcd() {
    next_screen_at = 0;
    on_screen = 0;  // Initialize to 0 (AKA screen_tilt)
    tilt_on_page = 0;
    tilt_pages_in_run = 0;

}  // bridge_lcd




void bridge_lcd::display_logo() {
#ifdef LCD_SSD1306
    // XBM files are C source bitmap arrays, and can be created in GIMP (and then read/imported using text editors)
    clear();
    oled_display->drawXbm((128-fermentrack_logo_width)/2, (64-fermentrack_logo_height)/2, fermentrack_logo_width, fermentrack_logo_height, fermentrack_logo_bits);
    display();
#endif

#ifdef LCD_TFT
    //print_line("Logo goes here.", "", 1);
    clear();
    tft->drawRGBBitmap((320-288)/2, 0, gimp_image.pixel_data, gimp_image.width, gimp_image.height);
#endif

#ifdef LCD_TFT_ESPI
    clear();
    tft->drawXBitmap((tft->width()-fermentrack_logo_width)/2, (tft->height()-fermentrack_logo_height)/2, fermentrack_logo_bits, fermentrack_logo_width, fermentrack_logo_height, TFT_WHITE);
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

    // Display IP address on bottom row if using Lolin TFT
    uint8_t header_row = 1;
    uint8_t first_tilt_row_offset = 2; 
#ifdef LCD_TFT
    // Display IP address or indicate if not connected.
    if ( WiFi.status() == WL_CONNECTED) {
        char ip[16];
        sprintf(ip, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3] );
        print_line("IP Address:", ip, 13);
    }
    else {
        print_line("No Wifi Connection","",13);
    }
    header_row = 1;
    first_tilt_row_offset = header_row + 1;
#endif

    // Display the header row
    print_line("Color", "Temp", "Gravity", header_row);

    // Loop through each of the tilt colors cached by tilt_scanner, searching for active tilts
    for(uint8_t i = 0;i<TILT_COLORS;i++) {
        if(tilt_scanner.tilt(i)->is_loaded()) {
            active_tilts++;
            // This check has the added bonus of limiting the # of displayed tilts to TILTS_PER_PAGE
            if((active_tilts/TILTS_PER_PAGE)==screen_number) {
                print_tilt_to_line(tilt_scanner.tilt(i), displayed_tilts + first_tilt_row_offset);
                displayed_tilts++;
            }
        }
    }

    // Toggle the actual display
    display();
}


void bridge_lcd::display_wifi_connect_screen(const char * ap_name, const char * ap_pass) {
    // This screen is displayed when the user first plugs in an unconfigured TiltBridge
    clear();
    print_line("To configure, connect to", "", 1);
    print_line("this AP via WiFi:", "", 2);
    print_line("Name:", ap_name, 3);
    print_line("Pass: ", ap_pass, 4);
    display();
}

void bridge_lcd::display_wifi_success_screen(const char* mdns_url, const char* ip_address_url) {
    // This screen is displayed at startup when the TiltBridge is configured to connect to WiFi
    clear();
#ifdef LCD_TFT_ESPI
    print_line("Access TiltBridge at:", "", 1);
#else
    print_line("Access your TiltBridge at:", "", 1);
#endif
    print_line(mdns_url, "", 2);
    print_line(ip_address_url, "", 3);
    display();
}

void bridge_lcd::display_wifi_reset_screen() {
    // When the user presses the "boot" switch, this screen appears. If the user presses the boot button a second time
    // while this screen is displayed, WiFi settings are cleared and the TiltBridge will return to displaying the
    // configuration AP at startup
    clear();
 
#if defined(LCD_SSD1306) || defined(LCD_TFT_ESPI)
    print_line("Press the button again to", "", 1);
    print_line("disable autoconnection", "", 2);
    print_line("and start the WiFi ", "", 3);
    print_line("configuration AP.", "", 4);
    display();
#endif

#ifdef LCD_TFT
    print_line("Tap the screen again to", "", 1);
    print_line("delete any saved WiFi", "", 2);
    print_line("credentials and restart", "", 3);
    print_line("the WiFi configuration AP", "", 4);
#endif

}

void bridge_lcd::display_ota_update_screen() {
    // When the user presses the "boot" switch, this screen appears. If the user presses the boot button a second time
    // while this screen is displayed, WiFi settings are cleared and the TiltBridge will return to displaying the
    // configuration AP at startup
#ifndef DISABLE_OTA_UPDATES
    clear();
    print_line("The TiltBridge firmware is", "", 1);
    print_line("being updated. Please do", "", 2);
    print_line("not power down or reset", "", 3);
    print_line("your TiltBridge.", "", 4);
    display();
#endif
}

void bridge_lcd::display_wifi_disconnected_screen() {
    // If the user's WiFi disconnects for any reason, it can take up to 20 seconds to reconnect. We'll print a message
    // letting the user know while we attempt to reconnect.
    clear();
    print_line("The TiltBridge has lost", "", 1);
    print_line("connection to your WiFi.", "", 2);
    print_line("", "", 3);
    print_line("Attempting to reconnect...", "", 4);
    display();
}

void bridge_lcd::display_wifi_reconnect_failed() {
    // If the user's WiFi disconnects for any reason, it can take up to 20 seconds to reconnect. We'll print a message
    // letting the user know while we attempt to reconnect.
    clear();
    print_line("The TiltBridge has lost", "", 1);
    print_line("connection to your WiFi.", "", 2);
    print_line("", "", 3);
    print_line("Attempting to reconnect...", "", 4);
    display();
}


void bridge_lcd::print_tilt_to_line(tiltHydrometer* tilt, uint8_t line) {
    char gravity[11], temp[8];
    sprintf(gravity, "%s", tilt->converted_gravity(false).c_str());
    sprintf(temp, "%s %s", tilt->converted_temp(false).c_str(), tilt->is_celsius() ? "C" : "F");

#ifdef LCD_TFT_ESPI
    tft->setTextColor(tilt->text_color());
#endif

    print_line(tilt->color_name().c_str(), temp, gravity, line);

#ifdef LCD_TFT_ESPI
    tft->setTextColor(TFT_WHITE);
#endif

}



#ifdef LCD_SSD1306
bool bridge_lcd::i2c_device_at_address(byte address, int sda_pin, int scl_pin) {
    // This allows us to do LCD autodetection (and by extension, support multiple OLED ESP32 boards
    byte error;

    Wire.begin(sda_pin, scl_pin);
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)  // No error means that a device responded
        return true;
    else
        return false;
}
#endif


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

#ifdef LCD_TFT
    tft = new Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
    tft->begin();
#ifdef LCD_TFT_M5_STACK
    // +4 "mirrors" the text (supposedly)
    tft->setRotation(0);
#else
    if(app_config.config["invertTFT"]){
        tft->setRotation(1);
    }
    else{
        tft->setRotation(3);
    }
#endif
    tft->fillScreen(ILI9341_BLACK);
    tft->setTextColor(ILI9341_WHITE, ILI9341_BLACK);

#if defined(TFT_BACKLIGHT)
    pinMode(TFT_BACKLIGHT, OUTPUT);
    digitalWrite(TFT_BACKLIGHT, HIGH);
#endif

#endif

#ifdef LCD_TFT_ESPI
    tft = new TFT_eSPI(TFT_WIDTH, TFT_HEIGHT);
    tft->init();
    tft->fontHeight(TFT_ESPI_FONT_HEIGHT);
    tft->setRotation(1);
    tft->fillScreen(TFT_BLACK);
#endif

}


void bridge_lcd::clear() {
#ifdef LCD_SSD1306
    oled_display->clear();
    oled_display->setFont(SSD1306_FONT);
#endif

#ifdef LCD_TFT
    tft->fillScreen(ILI9341_BLACK);
#endif

#ifdef LCD_TFT_ESPI
    tft->fillScreen(TFT_BLACK);
#endif

}

void bridge_lcd::display() {
#ifdef LCD_SSD1306
    oled_display->display();
#endif
}


void bridge_lcd::print_line(const char* left_text, const char* right_text, uint8_t line) {
#ifdef LCD_TFT_ESPI
    print_line("", left_text, right_text, line);
#else
    print_line(left_text, "", right_text, line);
#endif
}


void bridge_lcd::print_line(const char* left_text, const char* middle_text, const char* right_text, uint8_t line) {
#ifdef LCD_SSD1306
    int16_t starting_pixel_row = 0;

    starting_pixel_row = (SSD_LINE_CLEARANCE + SSD1306_FONT_HEIGHT) * (line-1) + SSD_LINE_CLEARANCE;

    // The coordinates define the left starting point of the text
    oled_display->setTextAlignment(TEXT_ALIGN_LEFT);
    oled_display->drawString(0, starting_pixel_row, left_text);

    oled_display->setTextAlignment(TEXT_ALIGN_LEFT);
    oled_display->drawString(48, starting_pixel_row, middle_text);

    oled_display->setTextAlignment(TEXT_ALIGN_RIGHT);
    oled_display->drawString(128, starting_pixel_row, right_text);
#endif


#ifdef LCD_TFT
    int16_t x = 0;
    int16_t y = TILT_FONT_SIZE * (line-1) * 9 + 2;

    tft->setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft->setTextSize(TILT_FONT_SIZE);

    tft->setCursor(x, y);
    tft->print(left_text);

    // For now, we're just dropping the middle text at pixel 130. No math.
    tft->setCursor(130, y);
    tft->print(middle_text);

    // While the OLED library has functions for printing right-aligned text, Adafruit GFX does not. We'll have to
    // do this manually.
    int16_t  x1, y1;
    uint16_t w, h;

    tft->getTextBounds(right_text, x, y, &x1, &y1, &w, &h);
    tft->setCursor(320-w,y);
    tft->print(right_text);

#endif

#ifdef LCD_TFT_ESPI
    // ignore left text as we color the text by the tilt
    int16_t starting_pixel_row = 0;

    starting_pixel_row = (TFT_ESPI_LINE_CLEARANCE + TFT_ESPI_FONT_SIZE) * (line-1) + TFT_ESPI_LINE_CLEARANCE;

    // TFT_eSPI::drawString(const char *string, int32_t poX, int32_t poY, uint8_t font_number)
    tft->drawString(middle_text, 0, starting_pixel_row, TFT_ESPI_FONT_NUMBER);
    tft->drawString(right_text, tft->width()/2, starting_pixel_row, TFT_ESPI_FONT_NUMBER);
#endif

}


