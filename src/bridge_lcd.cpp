//
// Created by John Beeler on 5/12/18.
//

#include "bridge_lcd.h"
#include "img/fermentrack_logo.h"

bridge_lcd lcd;


#ifdef LCD_SSD1306
#include <SSD1306.h>
#endif




bridge_lcd::bridge_lcd() {
    oled_display = new SSD1306(0x3c, 21, 22);

}  // bridge_lcd


void bridge_lcd::init() {
#ifdef LCD_SSD1306
    oled_display->init();

    oled_display->flipScreenVertically();
    oled_display->setFont(ArialMT_Plain_10);
#endif

}


void bridge_lcd::display_logo() {
    // XBM files are C source bitmap arrays, and can be created in GIMP (and then read/imported using text editors)
#ifdef LCD_SSD1306
    oled_display->clear();
    oled_display->drawXbm((128-fermentrack_logo_width)/2, (64-fermentrack_logo_height)/2, fermentrack_logo_width, fermentrack_logo_height, fermentrack_logo_bits);
    oled_display->display();
#endif
}


void bridge_lcd::display_tilts() {

    uint8_t active_tilts = 0;
    uint8_t displayed_tilts = 0;

    // If the page that was last displayed is the same as the total number of pages in this run (IE - if we've already
    // displayed all the pages that exist in this run) then reset both the on_page & pages_in_run counters.
    if(tilt_on_page == tilt_pages_in_run) {
        tilt_on_page = 0;
        tilt_pages_in_run = 0;
    }

    // Loop through each of the tilt colors cached by tilt_scanner, searching for active tilts
    for(uint8_t i = 0;i<TILT_COLORS;i++) {
        if(tilt_scanner.tilt(i)->is_loaded()) {
            active_tilts++;
            // This check has the added bonus of limiting the # of displayed tilts to TILTS_PER_PAGE
            if((active_tilts/TILTS_PER_PAGE)==tilt_on_page) {
                print_tilt_to_line(tilt_scanner.tilt(i), displayed_tilts+2);
                displayed_tilts++;
            }
        }
    }

    // If we reset the number of pages in the run above, we need to reinitialize it here.
    // NOTE - This code will only get used if "display tilts" is the only screen in use, as this same check will be
    // used to determine when to progress to the next screen
    if(tilt_pages_in_run == 0)
        tilt_pages_in_run = (active_tilts/TILTS_PER_PAGE) + 1;

    tilt_on_page++;
}



void bridge_lcd::print_tilt_to_line(tiltHydrometer* tilt, uint8_t line) {
    char gravity[10];
    sprintf(gravity, "%.3f", double_t(tilt->gravity)/1000);
    print_line(tilt->color_name().c_str(), gravity, line);
}



void bridge_lcd::print_line(String left_text, String right_text, uint8_t line) {
#ifdef LCD_SSD1306
    int16_t starting_pixel_row = 0;

    starting_pixel_row = (SSD_LINE_CLEARANCE + SSD1306_FONT_HEIGHT) * (line-1) + SSD_LINE_CLEARANCE;

    // TODO - Remove the clear from this function
    oled_display->clear();

    oled_display->setFont(SSD1306_FONT);

    // The coordinates define the left starting point of the text
    oled_display->setTextAlignment(TEXT_ALIGN_LEFT);
    oled_display->drawString(0, starting_pixel_row, left_text);

    oled_display->setTextAlignment(TEXT_ALIGN_RIGHT);
    oled_display->drawString(128, starting_pixel_row, right_text);

    oled_display->display();

#endif
}

