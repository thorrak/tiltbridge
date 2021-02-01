#include "watchButtons.h"

static unsigned long wifiButtonTime = 0;    // Button press timer
bool setWiFiPushed = false;          // Global button
bool firstWiFiPress = false;         // Button switch
bool doWiFiReset = false;            // Global semaphore (handle in loop())
bool doBoardReset = false;           // Global semaphore (handle in loop())

void IRAM_ATTR wifiButtonPressed() {
#ifndef LCD_TFT
    // When the wifi button is pressed, just log the time & get back to work
    if (millis() > wifiButtonTime + WIFIRESET_DEBOUNCE) {
        setWiFiPushed = true;
        wifiButtonTime = millis();        
    }
    // TODO:  Test this
#endif
}

void IRAM_ATTR boardButtonPressed() {
#ifndef LCD_TFT
    // When the (soft) reset button is pressed, there's no looking back
    doBoardReset = true;
#endif
}

void initWiFiResetButton() {
#ifndef LCD_TFT
    pinMode(WIFI_RESET_BUTTON_GPIO, INPUT_PULLUP);
    attachInterrupt(WIFI_RESET_BUTTON_GPIO, wifiButtonPressed, RISING);
#endif
}

void initBoardResetButton() {
#ifndef LCD_TFT
    pinMode(BOARD_RESET_BUTTON_GPIO, INPUT_PULLUP);
    attachInterrupt(BOARD_RESET_BUTTON_GPIO, boardButtonPressed, RISING);
#endif
}

void initButtons() {
    initWiFiResetButton();
    initBoardResetButton();
}

void checkButtons() {
    lcd.checkTouch();
    if (setWiFiPushed && ! firstWiFiPress) {
        // Log the first button click
        firstWiFiPress = true;
        setWiFiPushed = false;
        wifiButtonTime = millis();
        lcd.display_wifi_reset_screen();
    } else if (!doWiFiReset && setWiFiPushed && firstWiFiPress && wifiButtonTime + WIFIRESET_TIMEOUT > millis()) {
        // Do WiFi reset
        doWiFiReset = true;
        lcd.clear(); // Clear the screen
    } else if (! doWiFiReset && firstWiFiPress && wifiButtonTime + WIFIRESET_TIMEOUT <= millis()) {
        // Timed out, clearing register
        firstWiFiPress = false;
        lcd.display_logo(true); // Clear the screen
    }
}
