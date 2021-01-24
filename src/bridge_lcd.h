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

#define TILTS_PER_PAGE 15 // The actual number is one fewer than this - the first row is used for headers
#define TILT_FONT_SIZE 2
#define MIN_PRESSURE 2000

#elif defined(LCD_TFT_ESPI)

#include <TFT_eSPI.h>
#include <SPI.h>

#define TFT_ESPI_FONT_SIZE 20
#define TFT_ESPI_LINE_CLEARANCE 4
#define TFT_ESPI_FONT_HEIGHT 2
#define FF17 &FreeSans9pt7b
#define GFXFF 1
#define TILTS_PER_PAGE 5 // The actual number is one fewer than this - the first row is used for headers

#endif // LCD_SSD1306

#define SCREEN_TILT 0
#define SCREEN_LOGO 1
#define SCREEN_MAX 2

class bridge_lcd
{
public:
    bridge_lcd();

    void init();
    void reinit();
    void display_logo();
    void check_touch();

    void display_wifi_connect_screen(const char *ap_name, const char *ap_pass);
    void display_wifi_success_screen(const char *mdns_url, const char *ip_address_url);
    void display_wifi_reset_screen();
    void display_ota_update_screen();

    void display_wifi_disconnected_screen();
    void display_wifi_reconnect_failed();

    void print_line(const char *left_text, const char *right_text, uint8_t line);
    void print_line(const char *left_text, const char *middle_text, const char *right_text, uint8_t line);

    void check_screen();
    void clear();

private:
    uint8_t display_next();
    void display_tilt_screen(uint8_t screen_number);
    void print_tilt_to_line(tiltHydrometer *tilt, uint8_t line);
    bool i2c_device_at_address(byte address, int sda_pin, int scl_pin);
    void display();

#ifdef LCD_SSD1306
    SSD1306 *oled_display;
#elif defined(LCD_TFT)
    TFT_eSPI *tft;
#elif defined(LCD_TFT_ESPI)
    TFT_eSPI *tft;
#endif // LCD_SSD1306

    uint8_t tilt_pages_in_run;  // Number of pages in the current loop through the active tilts (# active tilts / 3)
    uint8_t tilt_on_page;       // The page number currently being displayed
    uint8_t on_screen;
    uint64_t next_screen_at;

    bool touchLatch = false;    // Ensure we only trigger a touch once
};

extern bridge_lcd lcd;
extern unsigned long wifiResetTime;

#endif // TILTBRIDGE_BRIDGE_LCD_H
