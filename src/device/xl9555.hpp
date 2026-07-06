#pragma once

#include "local/i2c.hpp"
#include "local/logger.hpp"

namespace wrapper
{

/**
 * @brief XL9555 16-bit I2C GPIO expander driver
 *
 * Provides 2 ports: P0 (pins 0-7) and P1 (pins 8-15).
 * Default I2C address: 0x20 (all address pins low).
 */
class Xl9555
{
   public:
    static constexpr uint8_t DEFAULT_ADDR = 0x20;
    static constexpr uint32_t DEFAULT_SPEED = 400000;

    // Register addresses
    static constexpr uint8_t REG_INPUT_P0 = 0x00;
    static constexpr uint8_t REG_INPUT_P1 = 0x01;
    static constexpr uint8_t REG_OUTPUT_P0 = 0x02;
    static constexpr uint8_t REG_OUTPUT_P1 = 0x03;
    static constexpr uint8_t REG_INV_P0 = 0x04;
    static constexpr uint8_t REG_INV_P1 = 0x05;
    static constexpr uint8_t REG_CONFIG_P0 = 0x06;
    static constexpr uint8_t REG_CONFIG_P1 = 0x07;

    // Direction constants
    static constexpr uint32_t DIR_OUTPUT = 0;
    static constexpr uint32_t DIR_INPUT = 1;

    explicit Xl9555(Logger& logger);
    ~Xl9555() = default;

    bool Init(const I2cBus& bus, const I2cDeviceConfig& config);
    bool Deinit();

    /** @brief Set pin direction (pin 0-15). DIR_OUTPUT=0, DIR_INPUT=1 */
    bool SetDirection(uint32_t io_num, uint32_t direction);

    /** @brief Set output level for a single pin (0-15). 0=LOW, 1=HIGH */
    bool SetLevel(uint32_t io_num, uint32_t level);

    /** @brief Read input level of a single pin (0-15). */
    bool GetLevel(uint32_t io_num, uint32_t* level);

    /** @brief Set all 16 output levels at once. bit[0..7]=P0, bit[8..15]=P1 */
    bool SetAllLevels(uint16_t value);

    /** @brief Set all 16 direction bits at once. 1=input, 0=output */
    bool SetAllDirections(uint16_t config);

    Logger& GetLogger() { return logger_; }

   private:
    Logger& logger_;
    I2cDevice device_;

    // Write-through cache for output and config registers
    uint8_t output_p0_{0xFF};
    uint8_t output_p1_{0xFF};
    uint8_t config_p0_{0xFF};
    uint8_t config_p1_{0xFF};
};

}  // namespace wrapper
