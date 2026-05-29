#include <esp_lcd_ili9881c.h>
#include <esp_lcd_touch_gt911.h>
#include "board/m5stack/tab5.hpp"
#include "device/ili9881c.hpp"
#include "device/gt911.hpp"

namespace wrapper
{
I2cBusConfig i2c_cfg(I2C_NUM_0,            // port
                     GPIO_NUM_31,          // sda (M5TAB5_I2C_SDA)
                     GPIO_NUM_32,          // scl (M5TAB5_I2C_SCL)
                     I2C_CLK_SRC_DEFAULT,  // clk_src
                     7,                    // glitch_ignore_cnt
                     0,                    // intr_priority
                     0,                    // trans_queue_depth (使用默认值)
                     true,                 // enable_internal_pullup
                     false                 // allow_pd
);

// M5Stack Tab5 背光配置
LedcTimerConfig ledc_timer_cfg(LEDC_LOW_SPEED_MODE,  // speed_mode
                               LEDC_TIMER_10_BIT,    // duty_resolution (0-1023)
                               LEDC_TIMER_0,         // timer_num
                               5000,                 // freq_hz (5kHz)
                               LEDC_AUTO_CLK         // clk_cfg
);

LedcChannelConfig ledc_channel_cfg(GPIO_NUM_22,          // gpio_num (M5TAB5_LCD_BACKLIGHT)
                                   LEDC_LOW_SPEED_MODE,  // speed_mode
                                   LEDC_CHANNEL_0,       // channel
                                   LEDC_INTR_DISABLE,    // intr_type
                                   LEDC_TIMER_0,         // timer_sel
                                   0,                    // duty (初始为0)
                                   0                     // hpoint
);

LdoChannelConfig dsi_phy_ldo_cfg(3,      // chan_id (LDO3 for MIPI DSI PHY)
                                 2500,   // voltage_mv (2.5V)
                                 false,  // adjustable
                                 false   // owned_by_hw
);

DsiBusConfig dsi_bus_cfg(0,                             // bus_id
                         2,                             // num_data_lanes
                         MIPI_DSI_PHY_CLK_SRC_DEFAULT,  // phy_clk_src
                         1000                           // lane_bit_rate_mbps
);

DsiDisplayConfig dsi_display_cfg(
    // DBI IO config parameters
    0,  // dbi_virtual_channel
    8,  // lcd_cmd_bits
    8,  // lcd_param_bits
    // DPI Panel config parameters
    0,                                   // dpi_virtual_channel
    MIPI_DSI_DPI_CLK_SRC_DEFAULT,        // dpi_clk_src
    60,                                  // dpi_clock_freq_mhz
    LCD_COLOR_PIXEL_FORMAT_RGB565,       // pixel_format
    static_cast<lcd_color_format_t>(0),  // in_color_format
    static_cast<lcd_color_format_t>(0),  // out_color_format
    1,                                   // num_fbs (官方默认值)
    // video_timing nested struct members
    720,   // h_size (BSP_LCD_H_RES)
    1280,  // v_size (BSP_LCD_V_RES)
    40,    // hsync_pulse_width
    140,   // hsync_back_porch
    40,    // hsync_front_porch
    4,     // vsync_pulse_width
    20,    // vsync_back_porch
    20,    // vsync_front_porch
    // flags nested struct members
    false,  // use_dma2d (单帧缓冲区时禁用以避免状态机冲突)
    false,  // disable_lp
    // Panel Device config parameters
    GPIO_NUM_NC,                // reset_gpio_num
    LCD_RGB_ELEMENT_ORDER_RGB,  // rgb_ele_order
    LCD_RGB_DATA_ENDIAN_BIG,    // data_endian
    16,                         // bits_per_pixel
    false                       // reset_active_high
);

I2cTouchConfig gt911_touch_cfg(ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP,
                               nullptr,
                               nullptr,
                               1,
                               0,
                               16,
                               0,
                               0,
                               0,
                               400000,
                               1280,
                               800,
                               GPIO_NUM_NC,
                               GPIO_NUM_NC,
                               0,
                               0,
                               0,
                               0,
                               0,
                               nullptr,
                               nullptr);

I2sBusConfig i2s_bus_cfg(I2S_NUM_0, I2S_ROLE_MASTER, 6, 256, true, false, 0);

I2sChanStdConfig tx_rx_std_cfg(
    // CLK config (5 params)
    48000,                  // sample_rate_hz
    I2S_CLK_SRC_DEFAULT,    // clk_src
    0,                      // ext_clk_freq_hz
    I2S_MCLK_MULTIPLE_256,  // mclk_multiple
    8,                      // bclk_div
    // SLOT config (10 params)
    I2S_DATA_BIT_WIDTH_16BIT,  // data_bit_width
    I2S_SLOT_BIT_WIDTH_AUTO,   // slot_bit_width
    I2S_SLOT_MODE_MONO,        // slot_mode
    I2S_STD_SLOT_BOTH,         // slot_mask
    16,                        // ws_width
    false,                     // ws_pol
    true,                      // bit_shift
    true,                      // msb_right
    false,                     // left_align
    false,                     // big_endian
    // GPIO config (8 params)
    GPIO_NUM_30,  // mclk
    GPIO_NUM_27,  // bclk
    GPIO_NUM_29,  // ws
    GPIO_NUM_26,  // dout
    GPIO_NUM_28,  // din
    false,        // invert_mclk
    false,        // invert_bclk
    false         // invert_ws
);

LvglPortConfig lvgl_port_cfg(5,                                      // task_prio
                             8192,                                   // stack_sz
                             1,                                      // affinity
                             20,                                     // max_sleep_ms
                             MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT,  // stack_caps
                             25                                      // timer_ms
);

LvglDisplayConfig lvgl_display_cfg(
    720 * 50,                // buf_sz (BSP_LCD_H_RES * CONFIG_BSP_LCD_DRAW_BUF_HEIGHT)
    true,                    // double_buf
    0,                       // trans_sz
    720,                     // hor_res
    1280,                    // ver_res
    false,                   // mono
    false,                   // swap_xy
    false,                   // mirror_x
    false,                   // mirror_y
    LV_COLOR_FORMAT_RGB565,  // format
    true,                    // buff_dma
    false,                   // buff_spiram
    true,                    // sw_rotate (官方默认启用)
    false,                   // swap_bytes (BSP_LCD_BIGENDIAN=0)
    false,                   // full_refresh
    false                    // direct_mode
);

LvglTouchConfig lvgl_touch_cfg(0.0f,  // scale_x
                               0.0f   // scale_y
);

LvglDisplayDsiConfig lvgl_dsi_cfg(false  // avoid_tearing (官方默认，需要num_fbs>1才能启用)
);

Logger lmain("Main");
Logger li2c("Board", "I2C", "Bus");
Logger lioexp0("Board", "I2C", "IoExpander0");
Logger lioexp1("Board", "I2C", "IoExpander1");
Logger lledc("Board", "LEDC");
Logger lldo("Board", "LDO");
Logger ldisp("Board", "Display");
Logger ltouch("Board", "Touch");
Logger laudio("Board", "Audio");
Logger llvgl("Board", "LVGL");

I2cBus i2c_bus(li2c);
Pi4ioe5v6408 io_expander0(lioexp0);  // 0x43
Pi4ioe5v6408 io_expander1(lioexp1);  // 0x44
LedcTimer ledc_timer(lledc);
LedcChannel ledc_channel(lledc);
LdoRegulator dsi_phy_ldo(lldo);

DsiBus dsi_bus(ldisp);
Ili9881c dsi_display(ldisp);
Gt911 gt911_touch(ltouch);

I2sBus i2s_bus(laudio);
LvglPort lvgl_port(llvgl);

AudioCodec audio_codec(laudio);
std::function<esp_err_t()> spk_codec_new_func = []() -> esp_err_t
{
    es8388_codec_cfg_t spk_codec_cfg = {
        .ctrl_if = audio_codec.GetSpeakerCtrlInterface(),
        .gpio_if = audio_codec.GetGpioInterface(),
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC,
        .master_mode = false,
        .pa_pin = GPIO_NUM_NC,
        .pa_reverted = false,
        .hw_gain = {.pa_voltage = 5.0, .codec_dac_voltage = 3.3, .pa_gain = 0.0f},
    };

    const audio_codec_if_t* codec_if = es8388_codec_new(&spk_codec_cfg);
    if (codec_if == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    audio_codec.SetSpeakerCodecInterface(codec_if);

    esp_codec_dev_handle_t spk_codec_dev_handle = nullptr;
    esp_codec_dev_cfg_t spk_codec_dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = codec_if,
        .data_if = audio_codec.GetDataInterface(),
    };
    spk_codec_dev_handle = esp_codec_dev_new(&spk_codec_dev_cfg);
    if (spk_codec_dev_handle == nullptr)
    {
        return ESP_ERR_INVALID_STATE;
    }
    audio_codec.SetSpeakerCodecDeviceHandle(spk_codec_dev_handle);
    return ESP_OK;
};
std::function<esp_err_t()> mic_codec_new_func = []() -> esp_err_t
{
    es7210_codec_cfg_t mic_codec_cfg = {.ctrl_if = audio_codec.GetMicrophoneCtrlInterface(),
                                        .master_mode = false,
                                        .mic_selected = ES7210_SEL_MIC1,
                                        .mclk_src = ES7210_MCLK_FROM_PAD,
                                        .mclk_div = 0};

    const audio_codec_if_t* codec_if = es7210_codec_new(&mic_codec_cfg);
    if (codec_if == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    audio_codec.SetMicrophoneCodecInterface(codec_if);
    esp_codec_dev_handle_t mic_codec_dev_handle = nullptr;
    esp_codec_dev_cfg_t mic_codec_dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_IN,
        .codec_if = codec_if,
        .data_if = audio_codec.GetDataInterface(),
    };
    mic_codec_dev_handle = esp_codec_dev_new(&mic_codec_dev_cfg);
    if (mic_codec_dev_handle == nullptr)
    {
        return ESP_ERR_INVALID_STATE;
    }
    audio_codec.SetMicrophoneCodecDeviceHandle(mic_codec_dev_handle);
    return ESP_OK;
};

bool M5StackTab5::Init(int level)
{
    if (level > 0)
    {
        // I2C Bus
        if (!i2c_bus.Init(i2c_cfg))
        {
            return false;
        }
        i2c_bus.Scan();

        // IO Expanders
        // Note: io_expander0.Init returns esp_err_t because it's Pi4ioe5v6408 which is not
        // refactored yet.
        if (!io_expander0.Init(i2c_bus, Pi4ioe5v6408::ADDR_LOW))
        {  // 0x43
            return false;
        }
        if (!io_expander1.Init(i2c_bus, Pi4ioe5v6408::ADDR_HIGH))
        {  // 0x44
            return false;
        }
    }

    if (level > 1)
    {
        // IO Expander0 配置 (LCD, Touch, Speaker, Camera)
        io_expander0.SetDirection(IO_EXPANDER_PIN_NUM_4, IO_EXPANDER_OUTPUT);  // LCD_EN
        io_expander0.SetLevel(IO_EXPANDER_PIN_NUM_4, 1);
        io_expander0.SetOutputMode(IO_EXPANDER_PIN_NUM_4, IO_EXPANDER_OUTPUT_MODE_PUSH_PULL);

        io_expander0.SetDirection(IO_EXPANDER_PIN_NUM_5, IO_EXPANDER_OUTPUT);  // TOUCH_EN
        io_expander0.SetLevel(IO_EXPANDER_PIN_NUM_5, 1);

        io_expander0.SetDirection(IO_EXPANDER_PIN_NUM_1, IO_EXPANDER_OUTPUT);  // SPEAKER_EN
        io_expander0.SetLevel(IO_EXPANDER_PIN_NUM_1, 1);

        io_expander1.SetDirection(IO_EXPANDER_PIN_NUM_3, IO_EXPANDER_OUTPUT);  // USB_EN
        io_expander1.SetLevel(IO_EXPANDER_PIN_NUM_3, 1);

        io_expander1.SetDirection(IO_EXPANDER_PIN_NUM_0, IO_EXPANDER_OUTPUT);  // WIFI_EN
        io_expander1.SetLevel(IO_EXPANDER_PIN_NUM_0, 1);

        // Backlight (LEDC)
        if (!ledc_timer.Init(ledc_timer_cfg))
        {
            return false;
        }
        if (!ledc_channel.Init(ledc_channel_cfg))
        {
            return false;
        }

        // DSI PHY Power (LDO)
        if (!dsi_phy_ldo.Init(dsi_phy_ldo_cfg))
        {
            return false;
        }

        // 等待 DSI PHY 电源稳定
        vTaskDelay(pdMS_TO_TICKS(100));

        dsi_bus.Init(dsi_bus_cfg);
        if (!dsi_display.Init(dsi_bus, dsi_display_cfg))
        {
            return false;
        }

        // 官方 BSP 在初始化后调用 InvertColor(false)
        dsi_display.InvertColor(false);

        if (!gt911_touch.Init(i2c_bus, gt911_touch_cfg))
        {
            return false;
        }

        if (!i2s_bus.Init(i2s_bus_cfg))
        {
            return false;
        }
        if (!i2s_bus.ConfigureTxChannel(tx_rx_std_cfg))
        {
            return false;
        }
        if (!i2s_bus.ConfigureRxChannel(tx_rx_std_cfg))
        {
            return false;
        }
        if (!i2s_bus.EnableTxChannel())
        {
            return false;
        }
        if (!i2s_bus.EnableRxChannel())
        {
            return false;
        }

        audio_codec.Init(i2s_bus);
        audio_codec.AddSpeaker(i2c_bus, ES8388_CODEC_DEFAULT_ADDR, spk_codec_new_func);
        audio_codec.AddMicrophone(i2c_bus, ES7210_CODEC_DEFAULT_ADDR, mic_codec_new_func);

        if (!lvgl_port.Init(lvgl_port_cfg))
        {
            return false;
        }
        if (!lvgl_port.AddDisplayDsi(dsi_display, lvgl_display_cfg, lvgl_dsi_cfg))
        {
            return false;
        }
        if (!lvgl_port.AddTouch(gt911_touch, lvgl_touch_cfg))
        {
            return false;
        }

        // 开启背光
        SetDisplayBrightness(100);
    }

    return true;
}

I2cBus& M5StackTab5::GetI2cBus() { return i2c_bus; }

Pi4ioe5v6408& M5StackTab5::GetIoExpander0() { return io_expander0; }

Pi4ioe5v6408& M5StackTab5::GetIoExpander1() { return io_expander1; }

LedcTimer& M5StackTab5::GetLedcTimer() { return ledc_timer; }

LedcChannel& M5StackTab5::GetLedcChannel() { return ledc_channel; }

LdoRegulator& M5StackTab5::GetDsiPhyLdo() { return dsi_phy_ldo; }

DsiBus& M5StackTab5::GetDsiBus() { return dsi_bus; }

Ili9881c& M5StackTab5::GetDsiDisplay() { return dsi_display; }

Gt911& M5StackTab5::GetGt911Touch() { return gt911_touch; }

I2sBus& M5StackTab5::GetI2sBus() { return i2s_bus; }

AudioCodec& M5StackTab5::GetAudioCodec() { return audio_codec; }

LvglPort& M5StackTab5::GetLvglPort() { return lvgl_port; }

void M5StackTab5::SetDisplayBrightness(int percent)
{
    if (percent < 0)
        percent = 0;
    if (percent > 100)
        percent = 100;

    // LEDC resolution set to 10bits (1023)
    uint32_t duty = (1023 * percent) / 100;
    ledc_channel.SetDutyAndUpdate(duty);
}

void M5StackTab5::SetDisplayBacklight(bool on) { SetDisplayBrightness(on ? 100 : 0); }

void M5StackTab5::SetDisplayPower(bool on)
{
    // IO_EXPANDER_PIN_NUM_4 is LCD_EN
    io_expander0.SetLevel(IO_EXPANDER_PIN_NUM_4, on ? 1 : 0);
}
}  // namespace wrapper