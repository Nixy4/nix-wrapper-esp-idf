#if __has_include("esp_lcd_touch_gt911.h")

#include "gt911.hpp"

namespace wrapper
{

bool Gt911::Init(const I2cBus& bus, const I2cTouchConfig& config)
{
    return I2cTouch::Init(bus, config, esp_lcd_touch_new_i2c_gt911);
}

}  // namespace wrapper

#endif