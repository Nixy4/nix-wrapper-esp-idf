#pragma once

#include "local/i2c.hpp"

namespace wrapper
{

class Axp2101 : public I2cDevice
{
   public:
    static constexpr uint8_t DEFAULT_ADDR = 0x34;
    static constexpr uint32_t DEFAULT_SPEED = 400000;

    Axp2101(Logger& logger);
    ~Axp2101();
};

}  // namespace wrapper
