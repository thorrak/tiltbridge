#include <driver/gpio.h>
#include <esp_timer.h>

#include "bridge_lcd.h"
#include "watchButtons.h"

// ESP-IDF replacement for Arduino millis()
static inline unsigned long millis() {
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
}

static unsigned long wifiButtonTime = 0;    // Button press timer
bool setWiFiPushed = false;          // Global button
bool firstWiFiPress = false;         // Button switch
bool doWiFiReset = false;            // Global semaphore (handle in loop())
bool doBoardReset = false;           // Global semaphore (handle in loop())

static void IRAM_ATTR wifiButtonPressed(void* arg) {
#ifndef LCD_TFT
    // When the wifi button is pressed, just log the time & get back to work
    if (millis() > wifiButtonTime + WIFIRESET_DEBOUNCE) {
        setWiFiPushed = true;
        wifiButtonTime = millis();
    }
#endif
}

static void IRAM_ATTR boardButtonPressed(void* arg) {
#ifndef LCD_TFT
    // When the (soft) reset button is pressed, there's no looking back
    doBoardReset = true;
#endif
}

void initWiFiResetButton() {
#ifndef LCD_TFT
#ifndef NO_BUTTONS
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << WIFI_RESET_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE,  // Rising edge
    };
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add((gpio_num_t)WIFI_RESET_BUTTON_GPIO, wifiButtonPressed, NULL);
#endif
#endif
}

void initBoardResetButton() {
#ifndef LCD_TFT
#ifndef NO_BUTTONS
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BOARD_RESET_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE,  // Rising edge
    };
    gpio_config(&io_conf);
    // ISR service already installed by initWiFiResetButton if called first
    gpio_isr_handler_add((gpio_num_t)BOARD_RESET_BUTTON_GPIO, boardButtonPressed, NULL);
#endif
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
