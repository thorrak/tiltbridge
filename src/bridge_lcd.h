//
// Created by John Beeler on 5/12/18.
//

#ifndef TILTBRIDGE_BRIDGE_LCD_H
#define TILTBRIDGE_BRIDGE_LCD_H

#include "tilt/tiltScanner.h"
#include <Arduino.h>


#ifdef LCD_SSD1306

#include <SSD1306.h>
#define SSD1306_FONT_HEIGHT     10
#define SSD_LINE_CLEARANCE      2
#define SSD1306_FONT            ArialMT_Plain_10
#define TILTS_PER_PAGE          5  // The actual number is one fewer than this - the first row is used for headers

#elif defined(LCD_TFT)

// For the LCD_TFT displays, we're connecting via SPI
#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

// Pin definitions for TFT displays
#define TFT_CS 14  //for D32 Pro
#define TFT_DC 27  //for D32 Pro
#define TFT_RST 33 //for D32 Pro
#define TS_CS  12 //for D32 Pro
#define TFT_BACKLIGHT 32
// TODO - Determine if I can actually use 15 tilts/display
#define TILTS_PER_PAGE          15  // The actual number is one fewer than this - the first row is used for headers
#define TILT_FONT_SIZE          2

#endif



#define SCREEN_TILT             0
#define SCREEN_FERMENTRACK      1

#define SCREEN_MAX              2

class bridge_lcd {
public:
    bridge_lcd();

    void init();
    void display_logo();

    void display_wifi_connect_screen(String ap_name, String ap_pass);
    void display_wifi_success_screen(const String& mdns_url, const String& ip_address_url);
    void display_wifi_reset_screen();
    void display_ota_update_screen();

    void print_line(const String& left_text, const String& right_text, uint8_t line);
    void print_line(const String& left_text, const String& middle_text, const String& right_text, uint8_t line);

    void check_screen();
    void clear();


private:
    uint8_t display_next();
    void display_tilt_screen(uint8_t screen_number);
    void print_tilt_to_line(tiltHydrometer* tilt, uint8_t line);

    void display();

#ifdef LCD_SSD1306
    SSD1306* oled_display;
#elif defined(LCD_TFT)
    Adafruit_ILI9341* tft;
#endif

    uint8_t tilt_pages_in_run;  // Number of pages in the current loop through the active tilts (# active tilts / 3)
    uint8_t tilt_on_page;       // The page number currently being displayed

    uint8_t on_screen;

    uint64_t next_screen_at;

#ifdef LCD_SSD1306
    bool i2c_device_at_address(byte address, int sda_pin, int scl_pin);
#endif

};

extern bridge_lcd lcd;
#endif //TILTBRIDGE_BRIDGE_LCD_H
