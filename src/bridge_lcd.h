//
// Created by John Beeler on 5/12/18.
//

#ifndef TILTBRIDGE_BRIDGE_LCD_H
#define TILTBRIDGE_BRIDGE_LCD_H

#include "tilt/tiltScanner.h"
#include "jsonconfig.h"
#include <Arduino.h>

#ifdef LCD_SSD1306

#include <SSD1306.h>
#define SSD1306_FONT_HEIGHT 10
#define SSD_LINE_CLEARANCE 2
#define SSD1306_FONT ArialMT_Plain_10
#define TILTS_PER_PAGE 5 // The actual number is one fewer than this - the first row is used for headers

#elif defined(LCD_TFT)

// For the LCD_TFT displays, we're connecting via SPI
#include <SPI.h>
#include <TFT_eSPI.h>
/*
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// Pin definitions for TFT displays
#define TFT_CS 14  //for D32 Pro
#define TFT_DC 27  //for D32 Pro
#define TFT_RST 33 //for D32 Pro
#define TS_CS 12   //for D32 Pro
#define TFT_BACKLIGHT 32
*/
// TODO - Determine if I can actually use 15 tilts/display
#define TILTS_PER_PAGE 15 // The actual number is one fewer than this - the first row is used for headers
#define TILT_FONT_SIZE 2

#elif defined(LCD_TFT_ESPI)

#include <TFT_eSPI.h>
#include <SPI.h>

#define TFT_ESPI_FONT_SIZE 25
#define TFT_ESPI_LINE_CLEARANCE 4
#define TFT_ESPI_FONT_HEIGHT 2
#define TFT_ESPI_FONT_NUMBER 4
#define TILTS_PER_PAGE 4 // The actual number is one fewer than this - the first row is used for headers

#endif

#define SCREEN_TILT 0
#define SCREEN_FERMENTRACK 1

#define SCREEN_MAX 2

class bridge_lcd
{
public:
    bridge_lcd();

    void init();
    void reinit();
    void display_logo();

    void display_wifi_connect_screen(const char *ap_name, const char *ap_pass);
    void display_wifi_success_screen(const char *mdns_url, const char *ip_address_url);
    void display_wifi_reset_screen();
    void display_ota_update_screen();

    void display_wifi_disconnected_screen();
    void display_wifi_reconnect_failed();

    void print_line(const char *left_text, const char *right_text, uint8_t line);
    void print_line(const char *left_text, const char *middle_text, const char *right_text, uint8_t line);

    //static void check_screen(void * parameter);
    void check_screen();
    void clear();

private:
    uint8_t display_next();
    void display_tilt_screen(uint8_t screen_number);
    void print_tilt_to_line(tiltHydrometer *tilt, uint8_t line);

    void display();

#ifdef LCD_SSD1306
    SSD1306 *oled_display;
#elif defined(LCD_TFT)
    TFT_eSPI *tft;
#elif defined(LCD_TFT_ESPI)
    TFT_eSPI *tft;
#endif
    uint8_t tilt_pages_in_run; // Number of pages in the current loop through the active tilts (# active tilts / 3)
    uint8_t tilt_on_page;      // The page number currently being displayed

    uint8_t on_screen;

    uint64_t next_screen_at;

#ifdef LCD_SSD1306
    bool i2c_device_at_address(byte address, int sda_pin, int scl_pin);
#endif
};

extern bridge_lcd lcd;
#endif //TILTBRIDGE_BRIDGE_LCD_H
