#pragma once

#include "local/display.hpp"

namespace wrapper
{

/**
 * @brief ST7796 SPI LCD panel driver
 *
 * 480x222 (landscape) / 222x480 (portrait) IPS display.
 * Uses the esp_lcd_new_panel_st7796 factory function ported from
 * .reference/LilyGoLib/src/bsp_lcd/esp_lcd_st7796.c.
 */
class St7796 : public SpiDisplay
{
   public:
    using SpiDisplay::SpiDisplay;  // Inherit constructor

    bool Init(const SpiBus& bus, const SpiDisplayConfig& config);
};

}  // namespace wrapper
