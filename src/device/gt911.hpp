#pragma once

#if __has_include("esp_lcd_touch_gt911.h")

#include "registry/touch.hpp"
#include "esp_lcd_touch_gt911.h"

namespace wrapper
{

class Gt911 : public I2cTouch
{
   public:
    using I2cTouch::I2cTouch;  // Inherit constructor

    bool Init(const I2cBus& bus, const I2cTouchConfig& config);
};

}  // namespace wrapper

#endif
