#include <cstdio>
#include <esp_timer.h>
#include <esp_log.h>
#include <thorlog.h>
#include <thorlog_espidf.h>

// TODO(ESP-IDF): Remove this include when migrating to pure ESP-IDF
#include <HardwareSerial.h>
// TODO(ESP-IDF): Add these includes instead:
// #include <driver/uart.h>

#include "serialhandler.h"

// Use the ESP-IDF print adapter from thorlog library
static EspIdfPrint espIdfAdapter;

// Get milliseconds since boot using ESP-IDF timer
static uint32_t millis_espidf() {
    return static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
}

void printTimestamp(ThorPrint *_logOutput)
{
    char c[12];
    sprintf(c, "%10lu ", (unsigned long)millis_espidf());
    _logOutput->print(c);
    fflush(stdout);
}

void printPrefix(ThorPrint* _logOutput, int logLevel) {
    printTimestamp(_logOutput);
}

void debug() {
    #if defined(LOG_LOCAL_LEVEL) && !defined(DISABLE_LOGGING)
    esp_log_level_set("*", ESP_LOG_WARN);

    esp_log_level_set("FreeRTOS", ESP_LOG_WARN);
    esp_log_level_set("NimBLE", ESP_LOG_WARN);
    esp_log_level_set("NIMBLE_NVS", ESP_LOG_WARN);
    esp_log_level_set("NimBLEAddress", ESP_LOG_WARN);
    esp_log_level_set("NimBLEAdvertisedDevice", ESP_LOG_WARN);
    esp_log_level_set("NimBLEAdvertising", ESP_LOG_WARN);
    esp_log_level_set("NimBLEAdvertisingReport", ESP_LOG_WARN);
    esp_log_level_set("NimBLEBeacon", ESP_LOG_WARN);
    esp_log_level_set("NimBLECharacteristic", ESP_LOG_WARN);
    esp_log_level_set("NimBLEClient", ESP_LOG_WARN);
    esp_log_level_set("NimBLEDescriptor", ESP_LOG_WARN);
    esp_log_level_set("NimBLEDevice", ESP_LOG_WARN);
    esp_log_level_set("NimBLEEddystoneTLM", ESP_LOG_WARN);
    esp_log_level_set("NimBLEEddystoneURL", ESP_LOG_WARN);
    esp_log_level_set("NimBLERemoteCharacteristic", ESP_LOG_WARN);
    esp_log_level_set("NimBLERemoteDescriptor", ESP_LOG_WARN);
    esp_log_level_set("NimBLERemoteService", ESP_LOG_WARN);
    esp_log_level_set("NimBLEScan", ESP_LOG_WARN);
    esp_log_level_set("NimBLEServer", ESP_LOG_WARN);
    esp_log_level_set("NimBLEService", ESP_LOG_WARN);
    esp_log_level_set("NimBLEUtils", ESP_LOG_WARN);
    esp_log_level_set("NimBLEUUID", ESP_LOG_WARN);

    esp_log_level_set("wifi", ESP_LOG_WARN);      // Enable WARN logs from WiFi stack
    esp_log_level_set("dhcpc", ESP_LOG_WARN);
#endif
}

void serial()
{
    // TODO(ESP-IDF): Replace Serial.begin/setDebugOutput with native UART init:
    //
    // const uart_config_t uart_config = {
    //     .baud_rate = BAUD,
    //     .data_bits = UART_DATA_8_BITS,
    //     .parity = UART_PARITY_DISABLE,
    //     .stop_bits = UART_STOP_BITS_1,
    //     .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    //     .source_clk = UART_SCLK_DEFAULT,
    // };
    // uart_param_config(UART_NUM_0, &uart_config);
    // uart_driver_install(UART_NUM_0, 256, 0, 0, NULL, 0);
    //
    // Note: ESP-IDF typically auto-configures UART0 for console output via
    // menuconfig (CONFIG_ESP_CONSOLE_UART), so explicit init may not be needed.

    Serial.begin(BAUD);
    Serial.setDebugOutput(true);

    printf("\n");
    fflush(stdout);
    Log.begin(ARDUINO_LOG_LEVEL, &espIdfAdapter, true);
    Log.setPrefix(printPrefix);
    Log.notice("Serial logging started at %l.\r\n", BAUD);

    debug();
}

size_t printDot()
{
    return printDot(false);
}

size_t printDot(bool safe)
{
#ifdef ARDUINO_LOG_LEVEL
    int result = printf(".");
    return (result > 0) ? static_cast<size_t>(result) : 0;
#else
    return 0;
#endif
}

size_t printChar(const char *chr)
{
    return printChar(false, chr);
}

size_t printChar(bool safe, const char *chr)
{
#ifdef ARDUINO_LOG_LEVEL
    int result = printf("%s\n", chr);
    return (result > 0) ? static_cast<size_t>(result) : 0;
#else
    return 0;
#endif
}

size_t printCR()
{
    return printCR(false);
}

size_t printCR(bool safe)
{
#ifdef ARDUINO_LOG_LEVEL
    int result = printf("\n");
    return (result > 0) ? static_cast<size_t>(result) : 0;
#else
    return 0;
#endif
}

void flush()
{
    flush(false);
}

void flush(bool safe)
{
    // TODO(ESP-IDF): Can optionally use uart_wait_tx_done(UART_NUM_0, portMAX_DELAY)
    // for guaranteed flush, but fflush(stdout) should work in most cases.
    fflush(stdout);
}
