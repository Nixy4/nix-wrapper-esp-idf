#pragma once

#include "esp_lcd_touch.h"
#include "esp_lcd_panel_io.h"
#include "wrapper/i2c.hpp"
#include "wrapper/logger.hpp"
#include <functional>

namespace wrapper
{

struct I2cTouchConfig
{
    esp_lcd_panel_io_i2c_config_t io_config;
    esp_lcd_touch_config_t touch_config;

    I2cTouchConfig(
        // io_config parameters
        uint16_t dev_addr, esp_lcd_panel_io_color_trans_done_cb_t on_color_trans_done,
        void *user_ctx, unsigned int control_phase_bytes, unsigned int dc_bit_offset,
        unsigned int lcd_cmd_bits, unsigned int lcd_param_bits, unsigned int dc_low_on_data,
        unsigned int disable_control_phase, uint32_t scl_speed_hz,
        // touch_config parameters
        uint16_t x_max, uint16_t y_max, gpio_num_t rst_gpio_num, gpio_num_t int_gpio_num,
        uint8_t levels_reset, uint8_t levels_interrupt, unsigned int swap_xy, unsigned int mirror_x,
        unsigned int mirror_y,
        void (*process_coordinates)(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y,
                                    uint16_t *strength, uint8_t *point_num, uint8_t max_point_num),
        esp_lcd_touch_interrupt_callback_t interrupt_callback)
        : io_config{}, touch_config{}
    {
        // Initialize io_config
        this->io_config.dev_addr = dev_addr;
        this->io_config.on_color_trans_done = on_color_trans_done;
        this->io_config.user_ctx = user_ctx;
        this->io_config.control_phase_bytes = control_phase_bytes;
        this->io_config.dc_bit_offset = dc_bit_offset;
        this->io_config.lcd_cmd_bits = lcd_cmd_bits;
        this->io_config.lcd_param_bits = lcd_param_bits;
        this->io_config.flags.dc_low_on_data = dc_low_on_data;
        this->io_config.flags.disable_control_phase = disable_control_phase;
        this->io_config.scl_speed_hz = scl_speed_hz;

        // Initialize touch_config
        this->touch_config.x_max = x_max;
        this->touch_config.y_max = y_max;
        this->touch_config.rst_gpio_num = rst_gpio_num;
        this->touch_config.int_gpio_num = int_gpio_num;
        this->touch_config.levels.reset = levels_reset;
        this->touch_config.levels.interrupt = levels_interrupt;
        this->touch_config.flags.swap_xy = swap_xy;
        this->touch_config.flags.mirror_x = mirror_x;
        this->touch_config.flags.mirror_y = mirror_y;
        this->touch_config.process_coordinates = process_coordinates;
        this->touch_config.interrupt_callback = interrupt_callback;
    }
};

class I2cTouch
{
    Logger &logger_;
    esp_lcd_panel_io_handle_t io_handle_;
    esp_lcd_touch_handle_t touch_handle_;

   public:
    I2cTouch(Logger &logger);
    ~I2cTouch();
    esp_lcd_touch_handle_t GetHandle() const { return touch_handle_; }
    // ops
    bool Init(const I2cBus &bus, const I2cTouchConfig &config,
              std::function<esp_err_t(const esp_lcd_panel_io_handle_t,
                                      const esp_lcd_touch_config_t *, esp_lcd_touch_handle_t *)>
                  new_touch_func);
    bool Deinit();

    bool ReadData();
    bool GetData(esp_lcd_touch_point_data_t *data, uint8_t *point_cnt, uint8_t max_point_cnt);
    bool GetCoordinates(uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num,
                        uint8_t max_point_num);
};

}  // namespace wrapper
