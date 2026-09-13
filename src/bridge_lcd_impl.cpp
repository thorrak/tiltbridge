#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <driver/gpio.h>
#include <thorlog.h>

#if defined(LCD_SSD1306)
#include <driver/i2c_master.h>
#endif

#include "jsonconfig.h"

// ESP-IDF GPIO compatibility helpers (replacing Arduino's pinMode/digitalWrite)
#ifndef OUTPUT
#define OUTPUT GPIO_MODE_OUTPUT
#endif
#ifndef HIGH
#define HIGH 1
#endif
#ifndef LOW
#define LOW 0
#endif

static inline void pinMode(int pin, gpio_mode_t mode) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = mode,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
}

static inline void digitalWrite(int pin, int level) {
    gpio_set_level((gpio_num_t)pin, level);
}

// Replace Arduino yield() with FreeRTOS equivalent
static inline void yield() {
    taskYIELD();
}
#include "tilt/tiltScanner.h"
#include "bridge_lcd.h"

#if HAVE_LCD
#include "lovyan_config.h"

#if defined(LCD_SSD1306) || defined(LCD_SMALL_TFT)
#include "img/oled_logo.h" // Small logo
#elif defined(LCD_LARGE_TFT)
#include "img/tft_logo_rle.h" // Large logo, RLE16 (see tools/gen_logo_rle.py)
#endif
#endif // HAVE_LCD

#if defined(AXP192)
#include "axp192.h"  // ESP-IDF compatible AXP192 driver for M5StickC Plus
AXP192_Driver axp192_driver;
#endif

#if defined(ESP32S3) && defined(LCD_SMALL_TFT)
#include "m5pm1.h"   // ESP-IDF compatible M5PM1 driver for M5StickC S3
M5PM1_Driver m5pm1_driver;
#endif



////////////////////////////////////////////////////////////
// Public Methods
////////////////////////////////////////////////////////////


inline void bridge_lcd::init_power() {
#if defined(LCD_SMALL_TFT) && defined(AXP192)
    // Detect which small TFT hardware is present and handle power init.
    // Detection must happen here (before display init) because M5StickC Plus
    // requires AXP192 initialization for power before the display can work.
    bool has_axp192 = false;
    _small_tft_variant = LGFX_SmallTFT_Universal::detect(
        [](int sda, int scl) { return axp192_driver.detect(sda, scl); },
        has_axp192);

    if (has_axp192) {
        AXP192_InitDef initDef = {
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
        axp192_driver.begin(21, 22, initDef);
    }
    // Plus2 and TTGO backlights are handled by LovyanGFX Light_PWM
#elif defined(ESP32S3) && defined(LCD_SMALL_TFT)
    // ESP32-S3 small TFT: detect M5StickC S3 vs S3 T-Display
    bool has_m5pm1 = false;
    _s3_small_tft_variant = LGFX_S3_SmallTFT_Universal::detect(
        [](int sda, int scl) { return m5pm1_driver.detect(sda, scl); },
        has_m5pm1);

    if (has_m5pm1) {
        // M5StickC S3: init M5PM1 and enable LCD power rail
        m5pm1_driver.begin(47, 48);
        m5pm1_driver.enableLcdPower(true);
    } else {
        // S3 T-Display: power-on via GPIO 15
        pinMode(15, OUTPUT);
        digitalWrite(15, HIGH);
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
    int sda_pin = -1, scl_pin = -1;
    int reset_pin = -1;

#ifdef I2C_SDA_PIN
    // The user is explicitly supplying the SDA and SCL pins
#ifndef I2C_SCL_PIN
#error "If you define I2C_SDA_PIN, you must also define I2C_SCL_PIN"
#endif
    // If the user explicitly supplies an SDA/SCL pin, we'll use that
    sda_pin = I2C_SDA_PIN;
    scl_pin = I2C_SCL_PIN;

#ifdef I2C_RESET_PIN
    reset_pin = I2C_RESET_PIN;
    pinMode(I2C_RESET_PIN, OUTPUT);
    // Apparently for the Heltec boards you have to do this twice. Go figure.
    digitalWrite(I2C_RESET_PIN, LOW); // Set I2C_RESET_PIN low to reset OLED
    vTaskDelay(pdMS_TO_TICKS(200));
    digitalWrite(I2C_RESET_PIN, HIGH); // While OLED is running, must set I2C_RESET_PIN in high
    vTaskDelay(pdMS_TO_TICKS(200));
    digitalWrite(I2C_RESET_PIN, LOW); // Set I2C_RESET_PIN low to reset OLED
    vTaskDelay(pdMS_TO_TICKS(200));
    digitalWrite(I2C_RESET_PIN, HIGH); // While OLED is running, must set I2C_RESET_PIN in high
#endif

#else

    // We're currently supporting three sets of hardware - The ESP32 "OLED"
    // board, TTGO Boards, and the sleeve (which I think nobody uses)
    if (i2c_device_at_address(0x3c, 5, 4)) {
        // This is the ESP32 "OLED" board
        sda_pin = 5;
        scl_pin = 4;
    } else if (i2c_device_at_address(0x3c, 21, 22)) {
        // This is the "sleeve": address, SDA, SCK
        sda_pin = 21;
        scl_pin = 22;
    } else {
        // For the "TTGO" style OLED shields, you have to power a pin to run the backlight.
        pinMode(16, OUTPUT);
        digitalWrite(16, LOW); // Set GPIO16 low to reset OLED
        vTaskDelay(pdMS_TO_TICKS(50));
        digitalWrite(16, HIGH); // While OLED is running, must set GPIO16 in high
        if (i2c_device_at_address(0x3c, 4, 15)) {
            sda_pin = 4;
            scl_pin = 15;
        } else {
            digitalWrite(16, LOW);                    // We weren't able to find the TTGO board, so reset the pin

            pinMode(21, OUTPUT);
            digitalWrite(21, LOW); // Set GPIO21 low to reset OLED
            vTaskDelay(pdMS_TO_TICKS(50));
            digitalWrite(21, HIGH); // While OLED is running, must set GPIO21 in high
            if (i2c_device_at_address(0x3c, 17, 18)) {
                sda_pin = 17;
                scl_pin = 18;
            } else {
                digitalWrite(21, LOW);                    // We weren't able to find the TTGO board, so reset the pin
                // ... and just default to the "sleeve" configuration
                sda_pin = 21;
                scl_pin = 22;
            }
        }
    }
#endif

    // Create and configure the LovyanGFX SSD1306 display
    auto ssd1306_tft = new LGFX_SSD1306();
    ssd1306_tft->configure(sda_pin, scl_pin, 0x3C, reset_pin);
    tft = ssd1306_tft;

    tft->init();
    if(!config.invertTFT) {
        // Due to historical reasons, the "non-inverted" orientation is technically the one that has had
        // flipScreenVertically() called.
        tft->setRotation(2);
    } else {
        // config.invertTFT is set. Toggle the semaphore.
        tft->setRotation(0);
    }


#elif defined(LCD_SMALL_TFT) || defined(LCD_LARGE_TFT)
    // Initialize appropriate LovyanGFX configuration based on hardware
#if defined(ESP32S3)
    {
        auto s3_tft = new LGFX_S3_SmallTFT_Universal();
        s3_tft->configure(_s3_small_tft_variant);
        tft = s3_tft;
    }
#elif defined(LCD_LARGE_TFT)
    {
        auto uni_tft = new LGFX_TFT_Universal();
        uni_tft->configure();
        tft = uni_tft;
    }
#elif defined(LCD_SMALL_TFT)
    {
        auto small_tft = new LGFX_SmallTFT_Universal();
        small_tft->configure(_small_tft_variant);
        tft = small_tft;
    }
#endif

    tft->init();
    tft->setSwapBytes(true);
    reinit();

    tft->setFont(&FreeSans9pt7b);

#ifdef TFT_BACKLIGHT
    pinMode(TFT_BACKLIGHT, OUTPUT);
    digitalWrite(TFT_BACKLIGHT, HIGH);
#endif // TFT_BACKLIGHT

#endif // LCD_SMALL_TFT
}

void bridge_lcd::reinit() {
#if defined(LCD_LARGE_TFT) || defined(LCD_SMALL_TFT)
    clear();
    if (config.invertTFT) {
        tft->setRotation(1);
    } else {
        tft->setRotation(3);
    }
#elif defined (LCD_SSD1306)
    // We can only flip the screen, not determine the current orientation
    if(config.invertTFT) {
        tft->setRotation(0);
    } else {
        tft->setRotation(2);
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
#if defined(LCD_SMALL_TFT)
    print_line("", left_text, "", line);
#else
    print_line(left_text, "", "", line);
#endif
}

void bridge_lcd::print_line(const char *left_text, const char *right_text, uint8_t line) {
#if defined(LCD_SMALL_TFT)
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
    tft->setTextDatum(textdatum_t::top_left);
    tft->drawString(left_text, 0, starting_pixel_row);

    tft->setTextDatum(textdatum_t::top_left);
    tft->drawString(middle_text, 48, starting_pixel_row);

    tft->setTextDatum(textdatum_t::top_right);
    tft->drawString(right_text, 128, starting_pixel_row);
#elif defined(LCD_LARGE_TFT)
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
#elif defined(LCD_SMALL_TFT)
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
    tft->fillScreen(0x0000);  // Black color
#elif defined(LCD_LARGE_TFT) || defined(LCD_SMALL_TFT)
    tft->fillScreen(0x0000);  // Black color in 16-bit RGB565 format
#endif
    yield();
}


////////////////////////////////////////////////////////////
// Private Methods
////////////////////////////////////////////////////////////

void bridge_lcd::print_tilt_to_line(tiltHydrometer *tilt, uint8_t line) {
    char gravity[14], temp[9], temp_str[6];
    tilt->cal_smooth_gravity_display_str(gravity, sizeof(gravity));
    tilt->converted_temp(temp_str, 6, false);
    snprintf(temp, sizeof(temp), "%s %s", temp_str, tilt->is_celsius() ? "C" : "F");

#if defined(LCD_SMALL_TFT)
    tft->setTextColor(tilt_text_colors[tilt->m_color]);
#endif

    // Print line with gutter for the color block for TFT screens
    print_line(tilt_color_names[tilt->m_color], temp, gravity, line, true);

#ifdef LCD_LARGE_TFT
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
#elif defined(LCD_SMALL_TFT)
    tft->setTextColor(0xFFFF);  // White in RGB565
#endif
}

bool bridge_lcd::i2c_device_at_address(uint8_t address, int sda_pin, int scl_pin) {
#ifdef LCD_SSD1306
    // LCD autodetection using the new ESP-IDF 5.x I2C master driver API
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = (gpio_num_t)sda_pin,
        .scl_io_num = (gpio_num_t)scl_pin,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = {
            .enable_internal_pullup = true,
        },
    };

    i2c_master_bus_handle_t bus_handle;
    esp_err_t err = i2c_new_master_bus(&bus_config, &bus_handle);
    if (err != ESP_OK) {
        Log.error("Failed to create I2C bus on pin %d/%d\r\n", sda_pin, scl_pin);
        return false;
    }

    err = i2c_master_probe(bus_handle, address, pdMS_TO_TICKS(100));
    i2c_del_master_bus(bus_handle);

    if (err == ESP_OK)
        return true;
#endif
    return false;
}

void bridge_lcd::display() {
#ifdef LCD_SSD1306
    // LovyanGFX auto-flushes, no explicit display() call needed
#endif
}


void bridge_lcd::display_logo_internal() {
#ifdef LCD_SSD1306
    tft->drawXBitmap(
        (128 - oled_logo_width) / 2,
        (64 - oled_logo_height) / 2,
        oled_logo_bits,
        oled_logo_width,
        oled_logo_height,
        0xFFFF);  // White in monochrome
    display();
#elif defined(LCD_LARGE_TFT)
    // The logo is stored run-length encoded (8.9 KB instead of 135 KB of raw
    // RGB565). Walk the runs straight into writeColor(), which fills N pixels
    // of one color inside the address window -- no decode buffer needed.
    tft->startWrite();
    tft->setAddrWindow((320 - TFT_LOGO_WIDTH) / 2, 0, TFT_LOGO_WIDTH, TFT_LOGO_HEIGHT);
    for (uint16_t i = 0; i < TFT_LOGO_RUNS; i++) {
        tft->writeColor(tft_logo_rle_colors[i], tft_logo_rle_counts[i]);
    }
    tft->endWrite();
#elif defined(LCD_SMALL_TFT)
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

