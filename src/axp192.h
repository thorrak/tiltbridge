/**
 * @file axp192.h
 * @brief ESP-IDF compatible AXP192 power management driver
 *
 * Simplified driver for AXP192 PMU used in M5StickC Plus.
 * Uses ESP-IDF I2C driver directly instead of Arduino Wire library.
 */

#ifndef TILTBRIDGE_AXP192_H
#define TILTBRIDGE_AXP192_H

#include <driver/i2c.h>
#include <esp_err.h>

#define AXP192_DEFAULT_ADDRESS  0x34

/**
 * @brief Configuration structure for AXP192 initialization
 */
struct AXP192_InitDef {
    bool EXTEN;         // External power enable
    bool BACKUP;        // Backup battery enable
    int16_t DCDC1;      // DCDC1 voltage in mV (0 = off, 700-3500)
    int16_t DCDC2;      // DCDC2 voltage in mV (0 = off, 700-2275)
    int16_t DCDC3;      // DCDC3 voltage in mV (0 = off, 700-3500)
    int16_t LDO2;       // LDO2 voltage in mV (0 = off, 1800-3300)
    int16_t LDO3;       // LDO3 voltage in mV (0 = off, 1800-3300)
    int16_t GPIO0;      // GPIO0 voltage in mV (0 = off, 1800-3300)
    int16_t GPIO1;      // GPIO1 mode (-1 = skip)
    int16_t GPIO2;      // GPIO2 mode (-1 = skip)
    int16_t GPIO3;      // GPIO3 mode (-1 = skip)
    int16_t GPIO4;      // GPIO4 mode (-1 = skip)
};

/**
 * @brief AXP192 power management class using ESP-IDF I2C
 */
class AXP192_Driver {
public:
    /**
     * @brief Construct AXP192 driver
     * @param i2c_port I2C port number (I2C_NUM_0 or I2C_NUM_1)
     * @param addr I2C address (default 0x34)
     */
    AXP192_Driver(i2c_port_t i2c_port = I2C_NUM_1, uint8_t addr = AXP192_DEFAULT_ADDRESS);

    /**
     * @brief Initialize I2C and configure AXP192
     * @param sda_pin SDA GPIO pin
     * @param scl_pin SCL GPIO pin
     * @param init Configuration settings
     * @return true if successful
     */
    bool begin(int sda_pin, int scl_pin, const AXP192_InitDef& init);

    /**
     * @brief Check if AXP192 is present on I2C bus
     * @param sda_pin SDA GPIO pin
     * @param scl_pin SCL GPIO pin
     * @return true if device responds
     */
    bool detect(int sda_pin, int scl_pin);

    /**
     * @brief Clean up I2C driver
     */
    void end();

    // Voltage setters
    void setDCDC1(int16_t voltage_mv);
    void setDCDC2(int16_t voltage_mv);
    void setDCDC3(int16_t voltage_mv);
    void setLDO2(int16_t voltage_mv);
    void setLDO3(int16_t voltage_mv);
    void setGPIO0(int16_t voltage_mv);

    // Control functions
    void setEXTEN(bool enable);
    void setBACKUP(bool enable);

private:
    i2c_port_t m_i2c_port;
    uint8_t m_addr;
    bool m_initialized;

    /**
     * @brief Initialize I2C driver
     */
    esp_err_t initI2C(int sda_pin, int scl_pin);

    /**
     * @brief Write a byte to a register
     */
    esp_err_t writeRegister(uint8_t reg, uint8_t value);

    /**
     * @brief Read a byte from a register
     */
    esp_err_t readRegister(uint8_t reg, uint8_t* value);

    /**
     * @brief Set bits in a register using mask
     */
    esp_err_t setBits(uint8_t reg, uint8_t mask, uint8_t value);

    // Internal voltage setting helpers
    void setDCDC(uint8_t channel, int16_t voltage_mv);
    void setLDO(uint8_t channel, int16_t voltage_mv);
};

#endif // TILTBRIDGE_AXP192_H
