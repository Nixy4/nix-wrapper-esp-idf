#pragma once

#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_mipi_dsi.h"
#include "wrapper/logger.hpp"
#include "wrapper/display.hpp"
#include <functional>

namespace wrapper
{
// MIPI DSI Bus Configuration
struct DsiBusConfig : public esp_lcd_dsi_bus_config_t
{
    DsiBusConfig(int bus_id,
                 uint8_t num_data_lanes,
                 mipi_dsi_phy_clock_source_t phy_clk_src,
                 uint32_t lane_bit_rate_mbps)
        : esp_lcd_dsi_bus_config_t{}
    {
        this->bus_id = bus_id;
        this->num_data_lanes = num_data_lanes;
        this->phy_clk_src = phy_clk_src;
        this->lane_bit_rate_mbps = lane_bit_rate_mbps;
    }
};

// Integrated DSI Display Configuration (combines DBI IO + DPI Panel + Panel Device configs)
struct DsiDisplayConfig
{
    esp_lcd_dbi_io_config_t dbi_config;
    esp_lcd_dpi_panel_config_t dpi_config;
    esp_lcd_panel_dev_config_t panel_config;

    DsiDisplayConfig(
        // DBI IO config parameters
        uint8_t dbi_virtual_channel,
        int lcd_cmd_bits,
        int lcd_param_bits,
        // DPI Panel config parameters
        uint8_t dpi_virtual_channel,
        mipi_dsi_dpi_clock_source_t dpi_clk_src,
        uint32_t dpi_clock_freq_mhz,
        lcd_color_rgb_pixel_format_t pixel_format,
        lcd_color_format_t in_color_format,
        lcd_color_format_t out_color_format,
        uint8_t num_fbs,
        // video_timing nested struct members
        uint32_t h_size,
        uint32_t v_size,
        uint32_t hsync_pulse_width,
        uint32_t hsync_back_porch,
        uint32_t hsync_front_porch,
        uint32_t vsync_pulse_width,
        uint32_t vsync_back_porch,
        uint32_t vsync_front_porch,
        // flags nested struct members
        bool use_dma2d,
        bool disable_lp,
        // Panel Device config parameters
        int reset_gpio_num,
        lcd_rgb_element_order_t rgb_ele_order,
        lcd_rgb_data_endian_t data_endian,
        uint32_t bits_per_pixel,
        bool reset_active_high = false)
        : dbi_config{}, dpi_config{}, panel_config{}
    {
        // DBI IO config
        this->dbi_config.virtual_channel = dbi_virtual_channel;
        this->dbi_config.lcd_cmd_bits = lcd_cmd_bits;
        this->dbi_config.lcd_param_bits = lcd_param_bits;

        // DPI Panel config
        this->dpi_config.virtual_channel = dpi_virtual_channel;
        this->dpi_config.dpi_clk_src = dpi_clk_src;
        this->dpi_config.dpi_clock_freq_mhz = dpi_clock_freq_mhz;
        this->dpi_config.pixel_format = pixel_format;
        this->dpi_config.in_color_format = in_color_format;
        this->dpi_config.out_color_format = out_color_format;
        this->dpi_config.num_fbs = num_fbs;

        this->dpi_config.video_timing.h_size = h_size;
        this->dpi_config.video_timing.v_size = v_size;
        this->dpi_config.video_timing.hsync_pulse_width = hsync_pulse_width;
        this->dpi_config.video_timing.hsync_back_porch = hsync_back_porch;
        this->dpi_config.video_timing.hsync_front_porch = hsync_front_porch;
        this->dpi_config.video_timing.vsync_pulse_width = vsync_pulse_width;
        this->dpi_config.video_timing.vsync_back_porch = vsync_back_porch;
        this->dpi_config.video_timing.vsync_front_porch = vsync_front_porch;

        this->dpi_config.flags.use_dma2d = use_dma2d ? 1U : 0U;
        this->dpi_config.flags.disable_lp = disable_lp ? 1U : 0U;

        // Panel Device config
        this->panel_config.reset_gpio_num = reset_gpio_num;
        this->panel_config.rgb_ele_order = rgb_ele_order;
        this->panel_config.data_endian = data_endian;
        this->panel_config.bits_per_pixel = bits_per_pixel;
        this->panel_config.flags.reset_active_high = reset_active_high ? 1U : 0U;
        this->panel_config.vendor_config = nullptr;  // Set at runtime
    }
};

/**
 * @brief MIPI DSI Bus wrapper class
 */
class DsiBus
{
   private:
    Logger& logger_;
    esp_lcd_dsi_bus_handle_t bus_handle_ = nullptr;

   public:
    DsiBus(Logger& logger);
    ~DsiBus();

    bool Init(const DsiBusConfig& config);
    bool Deinit();

    Logger& GetLogger();
    esp_lcd_dsi_bus_handle_t GetHandle() const;
};

/**
 * @brief DSI Display wrapper class
 *
 * Inherits from DisplayBase in display-new.hpp
 */
class DsiDisplay : public DisplayBase
{
   private:
    // Logger& logger_;

    bool InitIo(const DsiBus& bus, const esp_lcd_dbi_io_config_t& config);
    bool InitPanel(
        const esp_lcd_panel_dev_config_t& panel_config,
        std::function<esp_err_t(const esp_lcd_panel_io_handle_t)> custom_init_panel_func = nullptr,
        void* vendor_config = nullptr,
        std::function<void(void)> vendor_config_init_func = nullptr);

   public:
    DsiDisplay(Logger& logger);
    ~DsiDisplay();
    // Operations
    bool Init(
        const DsiBus& bus,
        const DsiDisplayConfig& config,
        std::function<esp_err_t(const esp_lcd_panel_io_handle_t,
                                const esp_lcd_panel_dev_config_t*,
                                esp_lcd_panel_handle_t*)> new_panel_func,
        std::function<esp_err_t(const esp_lcd_panel_io_handle_t)> custom_init_panel_func = nullptr,
        void* vendor_config = nullptr,
        std::function<void(void)> vendor_config_init_func = nullptr);

    bool Deinit();
};
}  // namespace wrapper

// #endif