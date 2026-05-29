#pragma once

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_dev.h"
#include "esp_lcd_panel_ops.h"

#include "wrapper/logger.hpp"
#include "wrapper/i2c.hpp"
#include "wrapper/spi.hpp"
// #include "wrapper/display.hpp"

namespace wrapper
{
struct I2cDisplayConfig
{
    esp_lcd_panel_io_i2c_config_t io_config;
    esp_lcd_panel_dev_config_t panel_config;

    I2cDisplayConfig(
        // io_config parameters
        uint16_t dev_addr = 0x00,
        esp_lcd_panel_io_color_trans_done_cb_t on_color_trans_done = nullptr,
        void* user_ctx = nullptr,
        unsigned int control_phase_bytes = 1,
        unsigned int dc_bit_offset = 0,
        unsigned int lcd_cmd_bits = 8,
        unsigned int lcd_param_bits = 8,
        bool dc_low_on_data = false,
        bool disable_control_phase = true,
        uint32_t scl_speed_hz = 100000,
        // panel_config parameters
        gpio_num_t reset_gpio = GPIO_NUM_NC,
        lcd_rgb_element_order_t rgb_order = LCD_RGB_ELEMENT_ORDER_RGB,
        lcd_rgb_data_endian_t data_endian = LCD_RGB_DATA_ENDIAN_BIG,
        uint32_t bits_per_pixel = 1,
        bool reset_active_high = false,
        void* vendor_conf = nullptr)
        : io_config{}, panel_config{}
    {
        // Init io_config
        io_config.dev_addr = dev_addr;
        io_config.on_color_trans_done = on_color_trans_done;
        io_config.user_ctx = user_ctx;
        io_config.control_phase_bytes = control_phase_bytes;
        io_config.dc_bit_offset = dc_bit_offset;
        io_config.lcd_cmd_bits = lcd_cmd_bits;
        io_config.lcd_param_bits = lcd_param_bits;
        io_config.flags.dc_low_on_data = dc_low_on_data;
        io_config.flags.disable_control_phase = disable_control_phase;
        io_config.scl_speed_hz = scl_speed_hz;

        // Init panel_config
        panel_config.reset_gpio_num = reset_gpio;
        panel_config.rgb_ele_order = rgb_order;
        panel_config.data_endian = data_endian;
        panel_config.bits_per_pixel = bits_per_pixel;
        panel_config.flags.reset_active_high = reset_active_high;
        panel_config.vendor_config = vendor_conf;
    }
};

struct SpiDisplayConfig
{
    esp_lcd_panel_io_spi_config_t io_config;
    esp_lcd_panel_dev_config_t panel_config;

    SpiDisplayConfig(
        // io_config parameters
        int cs_gpio,
        int dc_gpio,
        int spi_mode,
        int clock_speed_hz,
        int lcd_cmd_bits,
        int lcd_param_bits,
        size_t trans_queue_depth = 10,
        esp_lcd_panel_io_color_trans_done_cb_t on_color_trans_done = nullptr,
        void* user_ctx = nullptr,
        int cs_ena_pretrans = 0,
        int cs_ena_posttrans = 0,
        // io_config flags
        unsigned int dc_high_on_cmd = 0,
        unsigned int dc_low_on_data = 0,
        unsigned int dc_low_on_param = 0,
        unsigned int octal_mode = 0,
        unsigned int quad_mode = 0,
        unsigned int sio_mode = 0,
        unsigned int lsb_first = 0,
        unsigned int cs_high_active = 0,
        // panel_config parameters
        gpio_num_t reset_gpio = GPIO_NUM_NC,
        lcd_rgb_element_order_t rgb_order = LCD_RGB_ELEMENT_ORDER_RGB,
        lcd_rgb_data_endian_t data_endian = LCD_RGB_DATA_ENDIAN_BIG,
        uint32_t bits_per_pixel = 16,
        bool reset_active_high = false,
        void* vendor_conf = nullptr)
        : io_config{}, panel_config{}
    {
        // Init io_config
        io_config.cs_gpio_num = static_cast<gpio_num_t>(cs_gpio);
        io_config.dc_gpio_num = static_cast<gpio_num_t>(dc_gpio);
        io_config.spi_mode = spi_mode;
        io_config.pclk_hz = clock_speed_hz;
        io_config.trans_queue_depth = trans_queue_depth;
        io_config.on_color_trans_done = on_color_trans_done;
        io_config.user_ctx = user_ctx;
        io_config.lcd_cmd_bits = lcd_cmd_bits;
        io_config.lcd_param_bits = lcd_param_bits;
        io_config.cs_ena_pretrans = cs_ena_pretrans;
        io_config.cs_ena_posttrans = cs_ena_posttrans;

        // Init flags
        io_config.flags.dc_high_on_cmd = dc_high_on_cmd;
        io_config.flags.dc_low_on_data = dc_low_on_data;
        io_config.flags.dc_low_on_param = dc_low_on_param;
        io_config.flags.octal_mode = octal_mode;
        io_config.flags.quad_mode = quad_mode;
        io_config.flags.sio_mode = sio_mode;
        io_config.flags.lsb_first = lsb_first;
        io_config.flags.cs_high_active = cs_high_active;

        // Init panel_config
        panel_config.reset_gpio_num = reset_gpio;
        panel_config.rgb_ele_order = rgb_order;
        panel_config.data_endian = data_endian;
        panel_config.bits_per_pixel = bits_per_pixel;
        panel_config.flags.reset_active_high = reset_active_high;
        panel_config.vendor_config = vendor_conf;
    }
};

class DisplayBase
{
   protected:
    esp_lcd_panel_io_handle_t io_handle_ = nullptr;
    esp_lcd_panel_handle_t panel_handle_ = nullptr;
    Logger& logger_;

   public:
    DisplayBase(esp_lcd_panel_io_handle_t io_handle,
                esp_lcd_panel_handle_t panel_handle,
                Logger& logger)
        : io_handle_(io_handle), panel_handle_(panel_handle), logger_(logger)
    {
    }
    DisplayBase(Logger& logger) : io_handle_(nullptr), panel_handle_(nullptr), logger_(logger) {}
    ~DisplayBase() = default;

    Logger& GetLogger() { return logger_; }

    // Panel IO operations
    bool IoTxParam(int lcd_cmd, const void* param, size_t param_size)
    {
        return esp_lcd_panel_io_tx_param(io_handle_, lcd_cmd, param, param_size) == ESP_OK;
    }

    bool IoTxColor(int lcd_cmd, const void* color, size_t color_size)
    {
        return esp_lcd_panel_io_tx_color(io_handle_, lcd_cmd, color, color_size) == ESP_OK;
    }

    // Panel operations
    bool Reset() { return esp_lcd_panel_reset(panel_handle_) == ESP_OK; }
    bool Init() { return esp_lcd_panel_init(panel_handle_) == ESP_OK; }

    bool DrawBitmap(int x_start, int y_start, int x_end, int y_end, const void* color_data)
    {
        return esp_lcd_panel_draw_bitmap(panel_handle_, x_start, y_start, x_end, y_end,
                                         color_data) == ESP_OK;
    }

    bool Mirror(bool mirror_x, bool mirror_y)
    {
        return esp_lcd_panel_mirror(panel_handle_, mirror_x, mirror_y) == ESP_OK;
    }

    bool SwapXY(bool swap_axes)
    {
        return esp_lcd_panel_swap_xy(panel_handle_, swap_axes) == ESP_OK;
    }

    bool SetGap(int x_gap, int y_gap)
    {
        return esp_lcd_panel_set_gap(panel_handle_, x_gap, y_gap) == ESP_OK;
    }

    bool InvertColor(bool invert_color_data)
    {
        return esp_lcd_panel_invert_color(panel_handle_, invert_color_data) == ESP_OK;
    }

    bool DispOnOff(bool on_off)
    {
        return esp_lcd_panel_disp_on_off(panel_handle_, on_off) == ESP_OK;
    }

    bool DispSleep(bool sleep) { return esp_lcd_panel_disp_sleep(panel_handle_, sleep) == ESP_OK; }

    // Resource cleanup
    bool DelPanel() { return esp_lcd_panel_del(panel_handle_) == ESP_OK; }
    bool DelIo() { return esp_lcd_panel_io_del(io_handle_) == ESP_OK; }

    // Handle getters
    esp_lcd_panel_io_handle_t GetIoHandle() const { return io_handle_; }
    esp_lcd_panel_handle_t GetPanelHandle() const { return panel_handle_; }
};

/**
 * @brief I2C LCD Display wrapper class
 *
 * Specialized display class for I2C-based LCD panels
 */
class I2cDisplay : public DisplayBase
{
   private:
    // const I2cBus &bus_;

    bool InitIo(const I2cBus& bus, const I2cDisplayConfig& config);
    bool InitPanel(
        const I2cDisplayConfig& config,
        std::function<esp_err_t(const esp_lcd_panel_io_handle_t)> custom_init_panel_func = nullptr);

   public:
    I2cDisplay(Logger& logger) : DisplayBase(nullptr, nullptr, logger) {}

    ~I2cDisplay() { Deinit(); }

    bool Init(
        const I2cBus& bus,
        const I2cDisplayConfig& config,
        std::function<esp_err_t(const esp_lcd_panel_io_handle_t,
                                const esp_lcd_panel_dev_config_t*,
                                esp_lcd_panel_handle_t*)> new_panel_func,
        std::function<esp_err_t(const esp_lcd_panel_io_handle_t)> custom_init_panel_func = nullptr);
    bool Deinit();
};

class SpiDisplay : public DisplayBase
{
   private:
    bool InitIo(const SpiBus& bus, const SpiDisplayConfig& config);
    bool InitPanel(
        const SpiDisplayConfig& config,
        std::function<esp_err_t(const esp_lcd_panel_io_handle_t)> custom_init_panel_func = nullptr);

   public:
    SpiDisplay(Logger& logger) : DisplayBase(nullptr, nullptr, logger) {}

    ~SpiDisplay() { Deinit(); }

    bool Init(
        const SpiBus& bus,
        const SpiDisplayConfig& config,
        std::function<esp_err_t(const esp_lcd_panel_io_handle_t,
                                const esp_lcd_panel_dev_config_t*,
                                esp_lcd_panel_handle_t*)> new_panel_func,
        std::function<esp_err_t(const esp_lcd_panel_io_handle_t)> custom_init_panel_func = nullptr);
    bool Deinit();
};

}  // namespace wrapper
