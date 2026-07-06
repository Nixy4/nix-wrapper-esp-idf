#pragma once

#include "local/i2c.hpp"

namespace wrapper
{

class Aw9523 : public I2cDevice
{
   public:
    static constexpr uint8_t DEFAULT_ADDR = 0x58;
    static constexpr uint32_t DEFAULT_SPEED = 400000;

    Aw9523(Logger& logger);
    ~Aw9523();
};

}  // namespace wrapper
