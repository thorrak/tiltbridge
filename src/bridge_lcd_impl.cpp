#include <ArduinoLog.h>
#include <WiFi.h>

#ifdef LCD_SSD1306
#include <Wire.h>
#endif

#include "jsonconfig.h"
#include "tilt/tiltScanner.h"
#include "bridge_lcd.h"

#if defined(LCD_SSD1306) || defined(LCD_TFT_ESPI)
#include "img/oled_logo.h" // Small logo
#elif defined(LCD_TFT)
#include "img/tft_logo.h" // Large logo
#endif

#if defined(AXP192)
#include <I2C_AXP192.h>  // This mostly applies to m5 Stick so we can control the TFT backlight
I2C_AXP192 axp192(I2C_AXP192_DEFAULT_ADDRESS, Wire1);
#endif


////////////////////////////////////////////////////////////
// Public Methods
////////////////////////////////////////////////////////////


inline void bridge_lcd::init_power() {
#ifdef AXP192
    // For m5 stick and whatnot, the LCD backlight AND the controller both are powered off the AXP192, so we need to initialize that first
    Wire1.begin(21, 22);    

    I2C_AXP192_InitDef initDef = {
        .EXTEN  = true,
        .BACKUP = true,
        .DCDC1  = 3300,
        .DCDC2  = 0,
        .DCDC3  = 0,
        .LDO2   = 3000,
        .LDO3   = 3000,
        .GPIO0  = 2800,
        .GPIO1  = -1,
        .GPIO2  = -1,
        .GPIO3  = -1,
        .GPIO4  = -1,
    };
    axp192.begin(initDef);
#endif

#ifdef PIN_POWER_ON
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);
#endif
}

void bridge_lcd::init() {
    init_power();

#ifdef LCD_SSD1306
    // For the OLED displays, we need to initialize the I2C bus -- but first, we need to figure out what pins to use
#ifdef I2C_SDA_PIN
    // The user is explicitly supplying the SDA and SCL pins
#ifndef I2C_SCL_PIN
#error "If you define I2C_SDA_PIN, you must also define I2C_SCL_PIN"
#endif
    // If the user explicitly supplies an SDA/SCL pin, we'll use that
    oled_display = new SSD1306Wire(0x3c, I2C_SDA_PIN, I2C_SCL_PIN);
#else

    // We're currently supporting three sets of hardware - The ESP32 "OLED"
    // board, TTGO Boards, and the sleeve (which I think nobody uses)
    if (i2c_device_at_address(0x3c, 5, 4)) {
        // This is the ESP32 "OLED" board
        oled_display = new SSD1306Wire(0x3c, 5, 4);
    } else if (i2c_device_at_address(0x3c, 21, 22)) {
        // This is the "sleeve": address, SDA, SCK
        oled_display = new SSD1306Wire(0x3c, 21, 22);
    } else {
        // For the "TTGO" style OLED shields, you have to power a pin to run the backlight.
        pinMode(16, OUTPUT);
        digitalWrite(16, LOW); // Set GPIO16 low to reset OLED
        delay(50);
        digitalWrite(16, HIGH); // While OLED is running, must set GPIO16 in high
        if (i2c_device_at_address(0x3c, 4, 15)) {
            oled_display = new SSD1306Wire(0x3c, 4, 15);
        } else {
            digitalWrite(16, LOW);                    // We weren't able to find the TTGO board, so reset the pin

            pinMode(21, OUTPUT); 
            digitalWrite(21, LOW); // Set GPIO16 low to reset OLED 
            delay(50); 
            digitalWrite(21, HIGH); // While OLED is running, must set GPIO16 in high 
            if (i2c_device_at_address(0x3c, 17, 18)) {
                oled_display = new SSD1306Wire(0x3c, 17, 18);
            } else {
                digitalWrite(21, LOW);                    // We weren't able to find the TTGO board, so reset the pin
                oled_display = new SSD1306Wire(0x3c, 21, 22); // ... and just default to the "sleeve" configuration
            }
        }
    }
#endif

    oled_display->init();
    if(!config.invertTFT) {
        // Due to historical reasons, the "non-inverted" orientation is technically the one that has had 
        // flipScreenVertically() called.
        oled_display->flipScreenVertically();
    } else {
        // config.invertTFT is set. Toggle the semaphore.
        oled_display->resetOrientation();
    }
    oled_display->setFont(ArialMT_Plain_10);


#elif defined(LCD_TFT_ESPI) || defined(LCD_TFT)
    tft = new TFT_eSPI(TFT_WIDTH, TFT_HEIGHT);
    tft->init();
    tft->setSwapBytes(true);
    reinit();

    tft->setFreeFont(FF_NORMAL);

#ifdef TFT_BACKLIGHT
    pinMode(TFT_BACKLIGHT, OUTPUT);
    digitalWrite(TFT_BACKLIGHT, HIGH);
#endif // TFT_BACKLIGHT

#endif // LCD_TFT_ESPI
}

void bridge_lcd::reinit() {
#if defined(LCD_TFT) || defined(LCD_TFT_ESPI)
    clear();
    if (config.invertTFT) {
        tft->setRotation(1);
    } else {
        tft->setRotation(3);
    }
#elif defined (LCD_SSD1306)
    // We can only flip the screen, not determine the current orientation
    if(config.invertTFT) {
        oled_display->resetOrientation();
    } else if (!config.invertTFT) {
        oled_display->flipScreenVertically();
    }
#endif
}


void bridge_lcd::checkTouch()
{
#ifdef TOUCH_CS

    uint16_t x = 0, y = 0; // Touch coordinates (not used here)
    bool touched = tft->getTouch(&x, &y, MIN_PRESSURE);

    if (touched && ! touchLatch && ! setWiFiPushed) {
        // New touch, not currently waiting to process a touch elsewhere
        touchLatch = true;
    } else if (touched && touchLatch) {
        // Same touch, do nothing
    } else if (! touched && touchLatch) {
        // Clear touchlatch, trigger a tap
        touchLatch = false;
        setWiFiPushed = true;
    } else {
        // On this day in history, nothing happened
        touchLatch = false;
    }
#endif
}


void bridge_lcd::print_line(const char *left_text, uint8_t line)
{
#if defined(LCD_TFT_ESPI)
    print_line("", left_text, "", line);
#else
    print_line(left_text, "", "", line);
#endif
}

void bridge_lcd::print_line(const char *left_text, const char *right_text, uint8_t line) {
#if defined(LCD_TFT_ESPI)
    print_line("", left_text, right_text, line);
#else
    print_line(left_text, "", right_text, line);
#endif
}

void bridge_lcd::print_line(const char *left_text, const char *middle_text, const char *right_text, uint8_t line) {
    print_line(left_text, middle_text, right_text, line, false);
}

void bridge_lcd::print_line(const char *left_text, const char *middle_text, const char *right_text, uint8_t line, bool add_gutter) {
#ifdef LCD_SSD1306
    int16_t starting_pixel_row = 0;

    starting_pixel_row = (SSD_LINE_CLEARANCE + SSD1306_FONT_HEIGHT) * (line - 1) + SSD_LINE_CLEARANCE;

    // The coordinates define the left starting point of the text
    oled_display->setTextAlignment(TEXT_ALIGN_LEFT);
    oled_display->drawString(0, starting_pixel_row, left_text);

    oled_display->setTextAlignment(TEXT_ALIGN_LEFT);
    oled_display->drawString(48, starting_pixel_row, middle_text);

    oled_display->setTextAlignment(TEXT_ALIGN_RIGHT);
    oled_display->drawString(128, starting_pixel_row, right_text);
#elif defined(LCD_TFT)
    int16_t starting_pixel_row = 0;
    starting_pixel_row = (tft->fontHeight(GFXFF)) * (line - 1) + 2;

    if(add_gutter)  // We need space to the left to be able to display the Tilt color block
        tft->drawString(left_text, 25, starting_pixel_row, GFXFF);
    else
        tft->drawString(left_text, 1, starting_pixel_row, GFXFF);

    yield();
    tft->drawString(middle_text, 134, starting_pixel_row, GFXFF);
    yield();
    if(add_gutter)
        tft->drawString(right_text, 300 - tft->textWidth(right_text, GFXFF), starting_pixel_row, GFXFF);
    else
        tft->drawString(right_text, 319 - tft->textWidth(right_text, GFXFF), starting_pixel_row, GFXFF);
#elif defined(LCD_TFT_ESPI)
    // ignore left text as we color the text by the tilt
    int16_t starting_pixel_row = 0;

    starting_pixel_row = (TFT_ESPI_LINE_CLEARANCE + TFT_ESPI_FONT_SIZE) * (line - 1) + TFT_ESPI_LINE_CLEARANCE;

    // TFT_eSPI::drawString(const char *string, int32_t poX, int32_t poY, uint8_t font_number)
    // TODO - Replace middle_text with left_text (and skip all middle text instead)
    tft->drawString(middle_text, 0, starting_pixel_row, GFXFF);
    tft->drawString(right_text, tft->width() / 2, starting_pixel_row, GFXFF);
#endif
}


void bridge_lcd::clear() {
#ifdef LCD_SSD1306
    oled_display->clear();
    oled_display->setFont(SSD1306_FONT);
#elif defined(LCD_TFT) || defined(LCD_TFT_ESPI)
    tft->fillScreen(TFT_BLACK);
#endif
    yield();
}


////////////////////////////////////////////////////////////
// Private Methods
////////////////////////////////////////////////////////////

void bridge_lcd::print_tilt_to_line(tiltHydrometer *tilt, uint8_t line) {
    char gravity[11], temp[9], temp_str[6];
    tilt->converted_gravity(gravity, 11, false);
    tilt->converted_temp(temp_str, 6, false);
    snprintf(temp, sizeof(temp), "%s %s", temp_str, tilt->is_celsius() ? "C" : "F");

#if defined(LCD_TFT_ESPI)
    tft->setTextColor(tilt_text_colors[tilt->m_color]);
#endif

    // Print line with gutter for the color block for TFT screens
    print_line(tilt_color_names[tilt->m_color], temp, gravity, line, true);

#ifdef LCD_TFT
    uint16_t fHeight = tft->fontHeight(GFXFF);
    if (tilt_text_colors[tilt->m_color] == 0xFFFF) { // White outline, black square
        tft->fillRect( // White square
            0,
            fHeight * (line - 1) + 2,
            15,
            fHeight - 8,
            TFT_WHITE);
        tft->fillRect( // Black square
            1,
            fHeight * (line - 1) + 3,
            13,
            fHeight - 10,
            TFT_BLACK);
    } else {
        // All else
        tft->fillRect(
            0,
            fHeight * (line - 1) + 2,
            15,
            fHeight - 8,
            tilt_text_colors[tilt->m_color]);
    }
#elif defined(LCD_TFT_ESPI)
    tft->setTextColor(TFT_WHITE);
#endif
}

bool bridge_lcd::i2c_device_at_address(byte address, int sda_pin, int scl_pin) {
#ifdef LCD_SSD1306
    // This allows us to do LCD autodetection (and by extension, support
    // multiple OLED ESP32 boards
    byte error;

   if(!Wire.begin(sda_pin, scl_pin)) {
        Log.error(F("Failed to initialize Wire on pin %d/%d\r\n"), sda_pin, scl_pin);
        return false;  // Failed to initialize twowire on selected sda/scl
   }
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    Wire.end();

    if (error == 0) // No error means that a device responded
        return true;
#endif // Leave this here to avoid compiler warning
    return false;
}

void bridge_lcd::display() {
#ifdef LCD_SSD1306
    oled_display->display();
#endif
}


void bridge_lcd::display_logo_internal() {
#ifdef LCD_SSD1306
    oled_display->drawXbm(
        (128 - oled_logo_width) / 2,
        (64 - oled_logo_height) / 2,
        oled_logo_width,
        oled_logo_height,
        oled_logo_bits);
    display();
#elif defined(LCD_TFT)
    tft->pushImage(
        (320 - 288) / 2, 0,
        gimp_image.width,
        gimp_image.height,
        gimp_image.pixel_data);
#elif defined(LCD_TFT_ESPI)
    tft->drawXBitmap(
        (tft->width() - oled_logo_width) / 2,
        (tft->height() - oled_logo_height) / 2,
        oled_logo_bits,
        oled_logo_width,
        oled_logo_height,
        TFT_WHITE);
    display();
#endif
}

