#pragma once
#include "wrapper/display.hpp"
#include "esp_lcd_ili9341.h"

namespace wrapper
{

class Ili9341 : public SpiDisplay
{
   public:
    using SpiDisplay::SpiDisplay;  // Inherit constructor

    bool Init(const SpiBus& buf, const SpiDisplayConfig& config);
};

}  // namespace wrapper
