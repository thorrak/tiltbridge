//
// Created by John Beeler on 5/12/18.
//

#ifndef TILTBRIDGE_BRIDGE_LCD_H
#define TILTBRIDGE_BRIDGE_LCD_H

#include "tilt/tiltScanner.h"
#include <Arduino.h>

#define LCD_SSD1306 1  // TODO - For Testing, delete

#ifdef LCD_SSD1306
#include <SSD1306.h>
#define SSD1306_FONT_HEIGHT     10
#define SSD_LINE_CLEARANCE      2
#define SSD1306_FONT            ArialMT_Plain_10
#endif


#define TILTS_PER_PAGE          5  // The actual number is one fewer than this - the first row is used for headers

#define SCREEN_TILT             0
#define SCREEN_FERMENTRACK      1

#define SCREEN_MAX              2

class bridge_lcd {
public:
    bridge_lcd();

    void init();
    void display_logo();

    void display_wifi_connect_screen(String ap_name, String ap_pass);
    void display_wifi_fail_screen();
    void display_wifi_success_screen(String mdns_url, String ip_address_url);

    void print_line(String left_text, String right_text, uint8_t line);

    void check_screen();


private:
    uint8_t display_next();
    void display_tilt_screen(uint8_t screen_number);
    void print_tilt_to_line(tiltHydrometer* tilt, uint8_t line);

    void clear();
    void display();

    SSD1306* oled_display;

    uint8_t tilt_pages_in_run;  // Number of pages in the current loop through the active tilts (# active tilts / 3)
    uint8_t tilt_on_page;       // The page number currently being displayed

    uint8_t on_screen;

    uint64_t next_screen_at;


};

extern bridge_lcd lcd;
#endif //TILTBRIDGE_BRIDGE_LCD_H
