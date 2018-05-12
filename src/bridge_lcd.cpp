//
// Created by John Beeler on 5/12/18.
//

#include "bridge_lcd.h"
#include "img/fermentrack_logo.h"

extern bridge_lcd lcd;


#ifdef LCD_SSD1306
#include <SSD1306.h>
SSD1306 display(0x3c, 21, 22);
#endif




bridge_lcd::bridge_lcd() {


}  // bridge_lcd


void bridge_lcd::init() {
#ifdef LCD_SSD1306
    display.init();

    display.flipScreenVertically();
    display.setFont(ArialMT_Plain_10);
#endif

}


void bridge_lcd::display_logo() {
    // see http://blog.squix.org/2015/05/esp8266-nodemcu-how-to-create-xbm.html
    // on how to create xbm files
    display.clear();
    display.drawXbm((128-fermentrack_logo_width)/2, (64-fermentrack_logo_height)/2, fermentrack_logo_width, fermentrack_logo_height, fermentrack_logo_bits);
    display.display();
}





