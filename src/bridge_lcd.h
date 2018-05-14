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


#define TILTS_PER_PAGE          3

#define LCD_MODE_TILT           1
#define LCD_MODE_FERMENTRACK    2


class bridge_lcd {
public:
    bridge_lcd();

    void init();
    void display_logo();


    void print_line(String left_text, String right_text, uint8_t line);

    void display_tilts();

private:
    void print_tilt_to_line(tiltHydrometer* tilt, uint8_t line);

    SSD1306* oled_display;


    uint8_t tilt_pages_in_run;  // Number of pages in the current loop through the active tilts (# active tilts / 3)
    uint8_t tilt_on_page;       // The page number currently being displayed


};

extern bridge_lcd lcd;
#endif //TILTBRIDGE_BRIDGE_LCD_H
