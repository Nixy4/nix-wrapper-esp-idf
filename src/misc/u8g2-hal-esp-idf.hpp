#pragma once
#include <u8g2.h>
#include "wrapper/logger.hpp"
#include "wrapper/i2c.hpp"
#include "wrapper/spi.hpp"

namespace wrapper
{
struct U8g2PortConfig:
{
};

class U8g2Port
{
    Logger& logger_;
    u8g2_t u8g2_;
    bool initialized_;

    uint8_t I2cByteCallback(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr);
    uint8_t SpiByteCallback(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr);
    uint8_t GpioAndDelayCallback(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr);

   public:
    U8g2Port(Logger& logger);
    ~U8g2Port();

    bool Init(const U8g2PortConfig& config);
    bool Deinit();
    u8g2_t* GetU8g2() { return &u8g2_; }
};

}  // namespace wrapper