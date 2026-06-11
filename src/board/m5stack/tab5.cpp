#include <esp_lcd_ili9881c.h>
#include <esp_lcd_touch_gt911.h>
#include <string>
#include "board/m5stack/tab5.hpp"
#include "device/ili9881c.hpp"
#include "device/gt911.hpp"
#include "device/m5stack_tab5_keyboard.hpp"

namespace wrapper
{
// I2C0: 板载总线 (SDA=GPIO31, SCL=GPIO32)
I2cBusConfig i2c0_cfg(I2C_NUM_0,            // port
                      GPIO_NUM_31,          // sda (M5TAB5_I2C_SDA)
                      GPIO_NUM_32,          // scl (M5TAB5_I2C_SCL)
                      I2C_CLK_SRC_DEFAULT,  // clk_src
                      7,                    // glitch_ignore_cnt
                      0,                    // intr_priority
                      0,                    // trans_queue_depth (使用默认值)
                      true,                 // enable_internal_pullup
                      false                 // allow_pd
);

// I2C1: 键盘专用总线 (SDA=GPIO0, SCL=GPIO1)
I2cBusConfig i2c1_cfg(I2C_NUM_1,            // port
                      GPIO_NUM_0,           // sda (Tab5 Keyboard SDA)
                      GPIO_NUM_1,           // scl (Tab5 Keyboard SCL)
                      I2C_CLK_SRC_DEFAULT,  // clk_src
                      7,                    // glitch_ignore_cnt
                      0,                    // intr_priority
                      0,                    // trans_queue_depth
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
    0,                             // dpi_virtual_channel
    MIPI_DSI_DPI_CLK_SRC_DEFAULT,  // dpi_clk_src
    60,                            // dpi_clock_freq_mhz
    LCD_COLOR_FMT_RGB565,  // in_color_format (帧缓冲格式，与 LVGL LV_COLOR_FORMAT_RGB565 一致)
    LCD_COLOR_FMT_RGB565,  // out_color_format (面板接受格式)
    1,                     // num_fbs (官方默认值)
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

I2sBusConfig i2s_bus_cfg(I2S_NUM_0,        // port
                         I2S_ROLE_MASTER,  // role
                         6,                // dma_desc_num (DMA 描述符数量)
                         256,              // dma_frame_num (每个 DMA 描述符帧数)
                         true,             // auto_clear_after_cb (回调后自动清除 DMA 缓冲区)
                         false,            // auto_clear_before_cb
                         0                 // intr_priority
);

// TX 和 RX 使用相同配置（ES8388 DAC 与 ES7210 ADC 参数一致，有意共用）
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

Logger li2c0("Board", "I2C0", "Bus");
Logger lioexp0("Board", "I2C0", "IoExpander0");
Logger lioexp1("Board", "I2C0", "IoExpander1");
Logger lledc("Board", "LEDC");
Logger lldo("Board", "LDO");
Logger ldisp("Board", "Display");
Logger ltouch("Board", "Touch");
Logger li2s("Board", "I2S", "Bus");
Logger laudio("Board", "Audio");
Logger llvgl("Board", "LVGL");
Logger li2c1("Board", "I2C1", "Bus");
Logger lkb("Board", "I2C1", "Keyboard");

// ── 总线实例 ──
I2cBus i2c0_bus(li2c0);
I2cBus i2c1_bus(li2c1);
I2sBus i2s_bus(li2s);

// ── 设备实例（IO Expander / 显示 / 触摸 / 键盘 / 背光 / LDO）──
Pi4ioe5v6408 io_expander0(lioexp0);  // I2C0 0x43
Pi4ioe5v6408 io_expander1(lioexp1);  // I2C0 0x44
M5Tab5Keyboard keyboard(lkb);        // I2C1 0x6D
LedcTimer ledc_timer(lledc);
LedcChannel ledc_channel(lledc);
LdoRegulator dsi_phy_ldo(lldo);
DsiBus dsi_bus(ldisp);
Ili9881c dsi_display(ldisp);
Gt911 gt911_touch(ltouch);

// ── 中间件实例（音频编解码 / LVGL）──
AudioCodec audio_codec(laudio);
LvglPort lvgl_port(llvgl);
/// @brief 扬声器编解码器（ES8388）工厂函数，由 AudioCodec::AddSpeaker 回调调用
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
/// @brief 麦克风编解码器（ES7210）工厂函数，由 AudioCodec::AddMicrophone 回调调用
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

bool M5StackTab5::InitCoreBusAndIoExpander()
{
    // I2C0 Bus
    if (!i2c0_bus.Init(i2c0_cfg))
    {
        return false;
    }
    i2c0_bus.Scan();

    // IO Expanders (Pi4ioe5v6408 尚未完成重构，Init 返回 esp_err_t)
    if (!io_expander0.Init(i2c0_bus, Pi4ioe5v6408::ADDR_LOW))  // 0x43
    {
        return false;
    }
    if (!io_expander1.Init(i2c0_bus, Pi4ioe5v6408::ADDR_HIGH))  // 0x44
    {
        return false;
    }

    // 与显示/音频无关的系统级电源引脚（IO Expander 初始化后立即使能）
    io_expander1.SetDirection(IO_EXPANDER_PIN_NUM_3, IO_EXPANDER_OUTPUT);  // USB_EN
    io_expander1.SetLevel(IO_EXPANDER_PIN_NUM_3, 1);
    io_expander1.SetDirection(IO_EXPANDER_PIN_NUM_0, IO_EXPANDER_OUTPUT);  // WIFI_EN
    io_expander1.SetLevel(IO_EXPANDER_PIN_NUM_0, 1);

    return true;
}

bool M5StackTab5::InitDisplay()
{
    // IO Expander0: LCD 和触摸相关引脚
    io_expander0.SetDirection(IO_EXPANDER_PIN_NUM_4, IO_EXPANDER_OUTPUT);  // LCD_EN
    io_expander0.SetLevel(IO_EXPANDER_PIN_NUM_4, 1);
    io_expander0.SetOutputMode(IO_EXPANDER_PIN_NUM_4, IO_EXPANDER_OUTPUT_MODE_PUSH_PULL);

    io_expander0.SetDirection(IO_EXPANDER_PIN_NUM_5, IO_EXPANDER_OUTPUT);  // TOUCH_EN
    io_expander0.SetLevel(IO_EXPANDER_PIN_NUM_5, 1);

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

    if (!dsi_bus.Init(dsi_bus_cfg))
    {
        return false;
    }
    if (!dsi_display.Init(dsi_bus, dsi_display_cfg))
    {
        return false;
    }

    // 官方 BSP 在初始化后调用 InvertColor(false)
    dsi_display.InvertColor(false);

    if (!gt911_touch.Init(i2c0_bus, gt911_touch_cfg))
    {
        return false;
    }

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

    // 开启背光（默认全亮；如需自定义初始亮度请在调用方调用 SetDisplayBrightness）
    SetDisplayBrightness(100);

    return true;
}

bool M5StackTab5::InitAudio()
{
    // IO Expander0: 扬声器上电
    io_expander0.SetDirection(IO_EXPANDER_PIN_NUM_1, IO_EXPANDER_OUTPUT);  // SPEAKER_EN
    io_expander0.SetLevel(IO_EXPANDER_PIN_NUM_1, 1);

    // I2S 总线
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

    // 音频编解码器
    audio_codec.Init(i2s_bus);
    audio_codec.AddSpeaker(i2c0_bus, ES8388_CODEC_DEFAULT_ADDR, spk_codec_new_func);
    audio_codec.AddMicrophone(i2c0_bus, ES7210_CODEC_DEFAULT_ADDR, mic_codec_new_func);

    return true;
}

I2cBus& M5StackTab5::GetI2cBus() { return i2c0_bus; }

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

bool M5StackTab5::InitKeyboard()
{
    if (!i2c1_bus.Init(i2c1_cfg))
    {
        return false;
    }

    if (!keyboard.Init(i2c1_bus))
    {
        return false;
    }

    uint8_t ver = 0;
    if (keyboard.GetVersion(ver))
    {
        lkb.Info("Firmware version: 0x%02X", ver);
    }

    return true;
}

M5Tab5Keyboard& M5StackTab5::GetKeyboard() { return keyboard; }

void M5StackTab5::TestKeyboard()
{
    if (!InitKeyboard())
    {
        lkb.Error("TestKeyboard: InitKeyboard failed");
        return;
    }

    // --- UI ---
    lv_obj_t* kb_label = nullptr;
    lv_obj_t* mode_label = nullptr;
    lvgl_port.Lock(0);
    {
        lv_obj_t* scr = lv_scr_act();

        lv_obj_t* title = lv_label_create(scr);
        lv_label_set_text(title, "Tab5 Keyboard Test  (SDA=0 SCL=1 INT=50)");
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

        mode_label = lv_label_create(scr);
        lv_label_set_text(mode_label, "Mode: Normal");
        lv_obj_align(mode_label, LV_ALIGN_TOP_LEFT, 16, 48);

        kb_label = lv_label_create(scr);
        lv_label_set_text(kb_label, "Waiting for key press...");
        lv_obj_set_width(kb_label, LV_PCT(90));
        lv_label_set_long_mode(kb_label, LV_LABEL_LONG_WRAP);
        lv_obj_align(kb_label, LV_ALIGN_CENTER, 0, 20);
    }
    lvgl_port.Unlock();

    keyboard.SetMode(M5Tab5KeyboardMode::String);

    lvgl_port.Lock(0);
    lv_label_set_text(mode_label, "Mode: String (Character)");
    lvgl_port.Unlock();

    // 必须先切换到 Custom 模式，否则固件 Binding 模式会覆盖颜色设置
    // (String 模式下固件 Binding: 右 LED 紫色、左 LED 熄灭)
    keyboard.SetRgbMode(M5Tab5RgbMode::Custom);
    keyboard.SetBrightness(10);
    keyboard.SetRgb(0, 0, 255, 0);  // 左 LED 绿色
    keyboard.SetRgb(1, 0, 0, 255);  // 右 LED 蓝色

    static std::string text_buf;

    keyboard.SetKeyCallback(
        [kb_label](const M5Tab5KeyEvent& e)
        {
            if (e.type == M5Tab5KeyboardMode::String)
            {
                const char* key = e.str_data;
                const bool has_ctrl = (e.str_modifier & 0x01u) != 0u;
                const bool has_alt = (e.str_modifier & 0x04u) != 0u;

                if (has_ctrl || has_alt)
                {
                    lkb.Info("Key: [%s%s%s]", has_ctrl ? "Ctrl+" : "", has_alt ? "Alt+" : "", key);
                }
                else
                {
                    lkb.Info("Key: \"%s\"", key);
                }

                if (e.str_len == 1)
                {
                    text_buf += key;
                }
                else
                {
                    text_buf += '[';
                    text_buf += key;
                    text_buf += ']';
                }

                if (text_buf.size() > 80)
                    text_buf = text_buf.substr(text_buf.size() - 80);

                lvgl_port.Lock(0);
                lv_label_set_text(kb_label, text_buf.c_str());
                lvgl_port.Unlock();
            }
        });

    lkb.Info("Keyboard ready. Polling every 50 ms (INT pin = GPIO50)");

    // GPIO50 (INT) 可用于未来的硬件中断集成；当前以固定间隔轮询
    for (;;)
    {
        keyboard.Poll();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
}  // namespace wrapper