#pragma once

#if __has_include("esp_io_expander.h")

#include "esp_io_expander.h"
#include "esp_io_expander_pi4ioe5v6408.h"
#include "local/i2c.hpp"

namespace wrapper
{

/**
 * @brief PI4IOE5V6408 I2C IO Expander wrapper class
 *
 * 8-bit I2C-bus and SMBus I/O port with interrupt and reset
 */
class Pi4ioe5v6408
{
   private:
    Logger& m_logger;
    esp_io_expander_handle_t m_handle;
    uint8_t m_dev_addr;

   public:
    static constexpr uint8_t ADDR_LOW = ESP_IO_EXPANDER_I2C_PI4IOE5V6408_ADDRESS_LOW;    // 0x43
    static constexpr uint8_t ADDR_HIGH = ESP_IO_EXPANDER_I2C_PI4IOE5V6408_ADDRESS_HIGH;  // 0x44
    static constexpr uint32_t DEFAULT_SPEED = 400000;

    Pi4ioe5v6408(Logger& logger);
    ~Pi4ioe5v6408();
    esp_io_expander_handle_t GetHandle() const { return m_handle; }

    // Ops
    bool Init(const I2cBus& bus, uint8_t dev_addr);
    bool Deinit();
    bool SetDirection(uint32_t io_num, uint32_t direction);
    bool SetLevel(uint32_t io_num, uint32_t level);
    bool GetLevel(uint32_t io_num, uint32_t* level);
    bool SetPullupMode(uint32_t io_num, uint32_t pull_mode);
    bool SetOutputMode(uint32_t io_num, esp_io_expander_output_mode_t mode);
    bool PrintState();
};

}  // namespace wrapper

#endif  // __has_include("esp_io_expander.h")
