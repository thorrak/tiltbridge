#include <thorlog.h>
#include <WiFi.h>

#if defined(LCD_SSD1306) || defined(LCD_TFT_M5STICKC)
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

#ifdef LCD_TFT_M5STICKC
bridge_lcd::M5Variant bridge_lcd::detect_m5_variant() {
    Wire1.begin(21, 22);
    Wire1.beginTransmission(0x34);  // AXP192 I2C address
    uint8_t error = Wire1.endTransmission();
    Wire1.end();

    if (error == 0) {
        Log.notice("Detected M5StickC Plus (AXP192 found)" CR);
        return M5Variant::Plus;
    } else {
        Log.notice("Detected M5StickC Plus2 (no AXP192)" CR);
        return M5Variant::Plus2;
    }
}
#endif


////////////////////////////////////////////////////////////
// Public Methods
////////////////////////////////////////////////////////////


inline void bridge_lcd::init_power() {
#ifdef LCD_TFT_M5STICKC
    m5_variant = detect_m5_variant();

    if (m5_variant == M5Variant::Plus) {
        // M5StickC Plus: Initialize AXP192 for power/backlight
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
    } else {
        // M5StickC Plus2: Set HOLD pin (GPIO4) HIGH to maintain power
        // Without this, the device may shut down on battery - see M5Stack docs
        // n.b - I ended up commenting this out as there was a weird 'clicking" noise when turning 
        //       off the device via the button when running on battery. I think the fix is to capture 
        //       the button press (GPIO 35?) and then set GPIO4 to low, but am fine with just disabling
        //       battery operation for now (which is what happens when these are commented out)
        //pinMode(4, OUTPUT);
        //digitalWrite(4, HIGH);

        // Turn on backlight via GPIO27
        pinMode(27, OUTPUT);
        digitalWrite(27, HIGH);
    }
#elif defined(PIN_POWER_ON)
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

#ifdef I2C_RESET_PIN
    pinMode(I2C_RESET_PIN, OUTPUT);
    // Apparently for the Heltec boards you have to do this twice. Go figure. 
    digitalWrite(I2C_RESET_PIN, LOW); // Set I2C_RESET_PIN low to reset OLED 
    delay(200); 
    digitalWrite(I2C_RESET_PIN, HIGH); // While OLED is running, must set I2C_RESET_PIN in high 
    delay(200); 
    digitalWrite(I2C_RESET_PIN, LOW); // Set I2C_RESET_PIN low to reset OLED 
    delay(200); 
    digitalWrite(I2C_RESET_PIN, HIGH); // While OLED is running, must set I2C_RESET_PIN in high 
#endif

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
            digitalWrite(21, LOW); // Set GPIO21 low to reset OLED 
            delay(50); 
            digitalWrite(21, HIGH); // While OLED is running, must set GPIO21 in high 
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
    // Initialize appropriate LovyanGFX configuration based on hardware
#if defined(LCD_TFT)
    tft = new LGFX_D32_Pro();
#elif defined(LCD_TFT_M5STICKC)
    {
        auto m5_tft = new LGFX_M5StickC();
        m5_tft->configure(m5_variant == M5Variant::Plus2);
        tft = m5_tft;
    }
#elif defined(ESP32S3)
    tft = new LGFX_S3_TDisplay();
    // tft = new LGFX();
#else
    tft = new LGFX_TFT_ESPI();
#endif
    
    tft->init();
    tft->setSwapBytes(true);
    reinit();

    tft->setFont(&FreeSans9pt7b);

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
    bool touched = tft->getTouch(&x, &y);

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
    starting_pixel_row = (tft->fontHeight()) * (line - 1) + 2;

    if(add_gutter)  // We need space to the left to be able to display the Tilt color block
        tft->drawString(left_text, 25, starting_pixel_row);
    else
        tft->drawString(left_text, 1, starting_pixel_row);

    yield();
    tft->drawString(middle_text, 134, starting_pixel_row);
    yield();
    if(add_gutter)
        tft->drawString(right_text, 300 - tft->textWidth(right_text), starting_pixel_row);
    else
        tft->drawString(right_text, 319 - tft->textWidth(right_text), starting_pixel_row);
#elif defined(LCD_TFT_ESPI)
    // ignore left text as we color the text by the tilt
    int16_t starting_pixel_row = 0;

    starting_pixel_row = (TFT_ESPI_LINE_CLEARANCE + TFT_ESPI_FONT_SIZE) * (line - 1) + TFT_ESPI_LINE_CLEARANCE;

    // LovyanGFX::drawString(const char *string, int32_t poX, int32_t poY)
    // TODO - Replace middle_text with left_text (and skip all middle text instead)
    tft->drawString(middle_text, 0, starting_pixel_row);
    tft->drawString(right_text, tft->width() / 2, starting_pixel_row);
#endif
}


void bridge_lcd::clear() {
#ifdef LCD_SSD1306
    oled_display->clear();
    oled_display->setFont(SSD1306_FONT);
#elif defined(LCD_TFT) || defined(LCD_TFT_ESPI)
    tft->fillScreen(0x0000);  // Black color in 16-bit RGB565 format
#endif
    yield();
}


////////////////////////////////////////////////////////////
// Private Methods
////////////////////////////////////////////////////////////

void bridge_lcd::print_tilt_to_line(tiltHydrometer *tilt, uint8_t line) {
    char gravity[11], temp[9], temp_str[6];
    tilt->cal_smooth_gravity_str(gravity, 11);
    tilt->converted_temp(temp_str, 6, false);
    snprintf(temp, sizeof(temp), "%s %s", temp_str, tilt->is_celsius() ? "C" : "F");

#if defined(LCD_TFT_ESPI)
    tft->setTextColor(tilt_text_colors[tilt->m_color]);
#endif

    // Print line with gutter for the color block for TFT screens
    print_line(tilt_color_names[tilt->m_color], temp, gravity, line, true);

#ifdef LCD_TFT
    uint16_t fHeight = tft->fontHeight();
    if (tilt_text_colors[tilt->m_color] == 0xFFFF) { // White outline, black square
        tft->fillRect( // White square
            0,
            fHeight * (line - 1) + 2,
            15,
            fHeight - 8,
            0xFFFF);  // White in RGB565
        tft->fillRect( // Black square
            1,
            fHeight * (line - 1) + 3,
            13,
            fHeight - 10,
            0x0000);  // Black in RGB565
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
    tft->setTextColor(0xFFFF);  // White in RGB565
#endif
}

bool bridge_lcd::i2c_device_at_address(byte address, int sda_pin, int scl_pin) {
#ifdef LCD_SSD1306
    // This allows us to do LCD autodetection (and by extension, support
    // multiple OLED ESP32 boards
    byte error;

   if(!Wire.begin(sda_pin, scl_pin)) {
        Log.error("Failed to initialize Wire on pin %d/%d\r\n", sda_pin, scl_pin);
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
        0xFFFF);  // White in RGB565
    display();
#endif
}

