//
// Created by John Beeler on 5/12/18.
//

#ifndef TILTBRIDGE_BRIDGE_LCD_H
#define TILTBRIDGE_BRIDGE_LCD_H

#include <Arduino.h>
#define LCD_SSD1306 1

#ifdef LCD_SSD1306
#include <SSD1306.h>
extern SSD1306 display;
#endif



class bridge_lcd {
public:
    bridge_lcd();

    void init();
    void display_logo();
};

extern bridge_lcd lcd;
#endif //TILTBRIDGE_BRIDGE_LCD_H
