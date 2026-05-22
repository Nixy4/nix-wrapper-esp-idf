#pragma once

#include "wrapper/display.hpp"

namespace wrapper
{

class Ssd1306 : public I2cDisplay
{
    // 成员变量使用 () 初始化在类内声明中会被解析为函数声明（most vexing parse）。
    // 需要改用 {} 大括号初始化
    I2cDisplayConfig config_{
        // io_config parameters
        0x3C,     // dev_addr
        nullptr,  // on_color_trans_done
        nullptr,  // user_ctx
        1,        // control_phase_bytes
        0,        // dc_bit_offset
        8,        // lcd_cmd_bits
        8,        // lcd_param_bits
        false,    // dc_low_on_data
        true,     // disable_control_phase
        400000,   // scl_speed_hz
        // panel_config parameters
        GPIO_NUM_NC,                // reset_gpio
        LCD_RGB_ELEMENT_ORDER_RGB,  // rgb_order
        LCD_RGB_DATA_ENDIAN_BIG,    // data_endian
        1,                          // bits_per_pixel
        false,                      // reset_active_high
        nullptr                     // vendor_conf
    };

   public:
    Ssd1306(wrapper::Logger& logger) : I2cDisplay(logger) {}
    ~Ssd1306() = default;
};

}  // namespace wrapper