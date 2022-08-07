#ifndef TILTBRIDGE_BRIDGE_LCD_H
#define TILTBRIDGE_BRIDGE_LCD_H

#define LOGO_TIME 2     // Time (in seconds) to display the logo
#define TILT_TIME 10    // Time (in seconds) to display the Tilt screen

#include "tilt/tiltScanner.h"
#include "jsonconfig.h"
#include <Arduino.h>

#ifdef LCD_SSD1306
#include <SSD1306Wire.h>
#define SSD1306_FONT_HEIGHT     10
#define SSD_LINE_CLEARANCE      2
#define SSD1306_FONT            ArialMT_Plain_10
#define TILTS_PER_PAGE          5 // The actual number is one fewer than this - the first row is used for headers

#elif defined(LCD_TFT)

// For the LCD_TFT displays, we're connecting via SPI
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#define TILTS_PER_PAGE          15 // The actual number is one fewer than this - the first row is used for headers
#define TILT_FONT_SIZE          2
#define MIN_PRESSURE            2000

#elif defined(LCD_TFT_ESPI)

#include <TFT_eSPI.h>
#include <SPI.h>

#define TFT_ESPI_FONT_SIZE      20
#define TFT_ESPI_LINE_CLEARANCE 4
#define TFT_ESPI_FONT_HEIGHT    2
#define FF17                    &FreeSans9pt7b
#define GFXFF                   1
#define TILTS_PER_PAGE          5 // The actual number is one fewer than this - the first row is used for headers

#elif defined(LCD_TFT_M5STICKC)

#include <M5StickC.h>

#define TFT_M5STICKC_LINE_CLEARANCE 0
#define GFXFF                   2
#define TILTS_PER_PAGE          5 // The actual number is one fewer than this - the first row is used for headers

#endif // LCD_SSD1306

#ifdef TILTS_PER_PAGE
#define HAVE_LCD                1
#else
#define HAVE_LCD                0
#endif

#define SCREEN_TILT             0
#define SCREEN_LOGO             1
#define SCREEN_MAX              2

class bridge_lcd {
public:
    bridge_lcd();

    void init();
    void reinit();
    void display_logo(bool fromReset = false);
    void checkTouch();

    void display_wifi_connect_screen(const char *ap_name, const char *ap_pass);
    void display_wifi_success_screen(const char *mdns_url, const char *ip_address_url);
    void display_wifi_reset_screen();
    void display_ota_update_screen();

    void display_wifi_disconnected_screen();
    void display_wifi_reconnect_failed();

    void print_line(const char *left_text, const char *right_text, uint8_t line);
    void print_line(const char *left_text, const char *middle_text, const char *right_text, uint8_t line);
    void print_line(const char *left_text, const char *middle_text, const char *right_text, uint8_t line, bool add_gutter);

    void check_screen();
    void clear();

private:
#if HAVE_LCD
    uint8_t display_next();
    void display_tilt_screen(uint8_t screen_number);
    void print_tilt_to_line(tiltHydrometer *tilt, uint8_t line);
    bool i2c_device_at_address(byte address, int sda_pin, int scl_pin);
    void display();

#ifdef LCD_SSD1306
    SSD1306Wire *oled_display;
#elif defined(LCD_TFT)
    Adafruit_ILI9341 *tft;
    // TODO - See if we can just update the below defines
#define TFT_WHITE ILI9341_WHITE
#define TFT_BLACK ILI9341_BLACK
#elif defined(LCD_TFT_ESPI) || defined(LCD_TFT_M5STICKC)
    TFT_eSPI *tft;
#endif // LCD_SSD1306

    uint8_t tilt_pages_in_run;  // Number of pages in the current loop through the active tilts (# active tilts / 3)
    uint8_t tilt_on_page;       // The page number currently being displayed
    uint8_t on_screen;
    unsigned long next_screen_at;

    bool touchLatch = false;    // Ensure we only trigger a touch once
#endif // HAVE_LCD
};

void screenFlip();

extern bridge_lcd lcd;
extern bool setWiFiPushed;

#endif // TILTBRIDGE_BRIDGE_LCD_H
