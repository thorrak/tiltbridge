#ifndef _WATCHBUTTONS_H
#define _WATCHBUTTONS_H

#include "bridge_lcd.h"
#include <ArduinoLog.h>
#include <Arduino.h>

// We use these for LCD_TFT_ESPI or LCD_SSD1306
#define BOARD_RESET_BUTTON_GPIO 35  // (Soft) Reset button
#define WIFI_RESET_BUTTON_GPIO 0    // Boot button
#define WIFIRESET_DEBOUNCE 100
#define BOARDRESET_DEBOUNCE 100

#define WIFIRESET_TIMEOUT 3000 // WiFi reset window (double-click timeout)
#define BOOTRESET_TIMEOUT 3000 // Board reset window (double-click timeout)

void initButtons();
void initWiFiResetButton();
void initBoardResetButton();
void checkButtons();

#endif // _WATCHBUTTONS_H
