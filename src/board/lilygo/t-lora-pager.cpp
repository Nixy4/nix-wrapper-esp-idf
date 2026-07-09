#include "sdkconfig.h"
#include "t-lora-pager.hpp"

#if CONFIG_WRAPPER_ESP32_BOARD_LILYGO_T_LORA_PAGER

#include <functional>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "es8311_codec.h"

#include "local/logger.hpp"
#include "local/i2c.hpp"
#include "local/spi.hpp"
#include "local/i2s.hpp"
#include "local/ledc.hpp"
#include "local/display.hpp"
#include "registry/audio.hpp"
#include "registry/lvgl.hpp"
#include "device/xl9555.hpp"
#include "device/st7796.hpp"
#include "device/tca8418.hpp"
#include "device/bq25896.hpp"
#include "device/lilygo_t_lora_pager_keyboard.hpp"

namespace wrapper
{

gpio_config_t boot_button_gpio_cfg = {.pin_bit_mask = (1ULL << GPIO_NUM_0),
                                      .mode = GPIO_MODE_INPUT,
                                      .pull_up_en = GPIO_PULLUP_ENABLE,
                                      .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                      .intr_type = GPIO_INTR_NEGEDGE};

static void boot_button_isr_handler(void* arg)
{
    auto* board = static_cast<LilyGoLoraPager*>(arg);
    if (board && gpio_get_level(GPIO_NUM_0) == 0)  // 确认低电平触发
    {
        board->shutdown_requested.store(true);
    }
    ESP_LOGE("LoraPager", "Boot button pressed, shutdown requested.");
}

// =============================================================================
// XL9555 虚拟引脚分配
// TODO: 请对照 T-LoraPager 原理图 PDF 确认具体编号。
//       (github.com/Xinyuan-LilyGO/LilyGoLib/blob/master/Files/...V1.0_20250805.pdf)
// =============================================================================
static constexpr uint32_t kXl9555PinKbRst = 0;    // TODO: 待原理图确认
static constexpr uint32_t kXl9555PinLoraEn = 1;   // TODO: 待原理图确认
static constexpr uint32_t kXl9555PinGpsEn = 2;    // TODO: 待原理图确认
static constexpr uint32_t kXl9555PinDrvEn = 3;    // TODO: 待原理图确认
static constexpr uint32_t kXl9555PinAmpEn = 4;    // TODO: 待原理图确认
static constexpr uint32_t kXl9555PinNfcEn = 5;    // TODO: 待原理图确认
static constexpr uint32_t kXl9555PinSdEn = 6;     // TODO: 待原理图确认
static constexpr uint32_t kXl9555PinSdDet = 7;    // TODO: 待原理图确认（输入）
static constexpr uint32_t kXl9555PinDispRst = 8;  // TODO: 待原理图确认（可选）

// ES8311 I2C 地址（esp_codec_dev 期望的8 位格式：7 位 0x18 << 1 = 0x30）
static constexpr uint8_t kEs8311Addr = 0x30;

// =============================================================================
// 总线配置
// =============================================================================

// I2C 总线（共用：XL9555、BQ25896、TCA8418、ES8311）
I2cBusConfig i2c_bus_cfg(I2C_NUM_0,            // port
                         GPIO_NUM_3,           // sda
                         GPIO_NUM_2,           // scl
                         I2C_CLK_SRC_DEFAULT,  // clk_src
                         7,                    // glitch_ignore_cnt
                         0,                    // intr_priority
                         0,                    // trans_queue_depth
                         true,                 // enable_internal_pullup
                         false                 // allow_pd
);

// 共享 SPI 总线（ST7796 显示屏、LoRa、NFC、SD）
SpiBusConfig spi_bus_cfg(SPI2_HOST,                     // host
                         GPIO_NUM_34,                   // mosi
                         GPIO_NUM_33,                   // miso
                         GPIO_NUM_35,                   // sclk
                         GPIO_NUM_NC,                   // quadwp
                         GPIO_NUM_NC,                   // quadhd
                         GPIO_NUM_NC,                   // data4
                         GPIO_NUM_NC,                   // data5
                         GPIO_NUM_NC,                   // data6
                         GPIO_NUM_NC,                   // data7
                         false,                         // data_default_level
                         480 * 222 * sizeof(uint16_t),  // max_transfer_sz
                         SPICOMMON_BUSFLAG_MASTER,      // bus_flags
                         ESP_INTR_CPU_AFFINITY_AUTO,    // isr_cpu_id
                         0,                             // intr_flags
                         SPI_DMA_CH_AUTO                // dma_chan
);

// ST7796 显示屏 SPI 面板 IO 配置
SpiDisplayConfig spi_display_cfg(
    // io_config
    GPIO_NUM_38,       // cs_gpio
    GPIO_NUM_37,       // dc_gpio
    0,                 // spi_mode
    40 * 1000 * 1000,  // clock_speed_hz (40 MHz)
    8,                 // lcd_cmd_bits
    8,                 // lcd_param_bits
    10,                // trans_queue_depth
    nullptr,           // on_color_trans_done
    nullptr,           // user_ctx
    0,                 // cs_ena_pretrans
    0,                 // cs_ena_posttrans
    // IO 配置标志
    0,  // dc_high_on_cmd
    0,  // dc_low_on_data
    0,  // dc_low_on_param
    0,  // octal_mode
    0,  // quad_mode
    0,  // sio_mode
    0,  // lsb_first
    0,  // cs_high_active
    // panel_dev_config
    GPIO_NUM_NC,                // reset_gpio_num（无专用硬件复位引脚）
    LCD_RGB_ELEMENT_ORDER_BGR,  // rgb_ele_order
    LCD_RGB_DATA_ENDIAN_BIG,    // data_endian
    16,                         // bits_per_pixel
    false,                      // reset_active_high
    nullptr                     // vendor_config
);

// LEDC 背光定时器（GPIO 42, 5 kHz, 10 位占空比 = 0–1023）
LedcTimerConfig ledc_timer_cfg(LEDC_LOW_SPEED_MODE,  // speed_mode
                               LEDC_TIMER_10_BIT,    // duty_resolution
                               LEDC_TIMER_0,         // timer_num
                               5000,                 // freq_hz
                               LEDC_AUTO_CLK         // clk_cfg
);

LedcChannelConfig ledc_channel_cfg(GPIO_NUM_42,          // gpio_num（显示屏背光）
                                   LEDC_LOW_SPEED_MODE,  // speed_mode
                                   LEDC_CHANNEL_0,       // channel
                                   LEDC_TIMER_0,         // timer_sel
                                   0,                    // duty（初始尚未亮起）
                                   0                     // hpoint
);

// I2S 总线（ES8311 音频编解码器）
I2sBusConfig i2s_bus_cfg(I2S_NUM_0,        // port
                         I2S_ROLE_MASTER,  // role
                         6,                // dma_desc_num
                         256,              // dma_frame_num
                         true,             // auto_clear_after_cb
                         false,            // auto_clear_before_cb
                         0                 // intr_priority
);

// I2S 标准通道 — TX（扬声器）与 RX（麦克风）共用
I2sChanStdConfig i2s_chan_cfg(
    // clk
    48000,                  // sample_rate_hz
    I2S_CLK_SRC_DEFAULT,    // clk_src
    0,                      // ext_clk_freq_hz
    I2S_MCLK_MULTIPLE_256,  // mclk_multiple
    8,                      // bclk_div
    // slot
    I2S_DATA_BIT_WIDTH_16BIT,  // data_bit_width
    I2S_SLOT_BIT_WIDTH_AUTO,   // slot_bit_width
    I2S_SLOT_MODE_MONO,        // slot_mode
    I2S_STD_SLOT_BOTH,         // slot_mask
    16,                        // ws_width
    false,                     // ws_pol
    true,                      // bit_shift
    true,                      // msb_right（ESP-IDF 参数顺序中即 left_align=true）
    false,                     // big_endian
    false,                     // bit_order_lsb
    // gpio
    GPIO_NUM_10,  // mclk
    GPIO_NUM_11,  // bclk
    GPIO_NUM_18,  // ws
    GPIO_NUM_45,  // dout（扬声器）
    GPIO_NUM_17,  // din （麦克风）
    false,        // invert_mclk
    false,        // invert_bclk
    false         // invert_ws
);

// XL9555 设备配置
I2cDeviceConfig xl9555_dev_cfg(Xl9555::DEFAULT_ADDR, Xl9555::DEFAULT_SPEED);

// BQ25896 设备配置
I2cDeviceConfig bq25896_dev_cfg(Bq25896::DEFAULT_ADDR, Bq25896::DEFAULT_SPEED);

// TCA8418 设备配置
I2cDeviceConfig tca8418_dev_cfg(Tca8418::DEFAULT_ADDR, Tca8418::DEFAULT_SPEED);

// LVGL 移植层
LvglPortConfig lvgl_port_cfg(5,                                    // task_priority
                             8192,                                 // task_stack
                             1,                                    // task_affinity
                             20,                                   // task_max_sleep_ms
                             MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT,  // task_stack_caps
                             25                                    // timer_period_ms
);

// LVGL 显示屏：480 × 222，RGB565
// x_offset = 0, y_offset = 49（ST7796 为 480×320 控制器，面板可视窗口从第 49 行开始）
LvglDisplayConfig lvgl_display_cfg(480 * 60,                // 缓冲区大小（一行 × 60 行）
                                   true,                    // double_buffer
                                   0,                       // trans_size
                                   480,                     // hres
                                   222,                     // vres
                                   false,                   // monochrome
                                   true,                    // swap_xy
                                   false,                   // mirror_x
                                   false,                   // mirror_y
                                   LV_COLOR_FORMAT_RGB565,  // color_format
                                   true,                    // buff_dma
                                   true,                    // buff_spiram
                                   false,                   // sw_rotate
                                   true,                    // swap_bytes（SPI 大端序）
                                   false,                   // full_refresh
                                   false                    // direct_mode
);

LvglTouchConfig lvgl_touch_cfg(0.0f, 0.0f);

// SdSpi — SPI 模式 SD 卡（CS=GPIO21）
// 卡检测（SD_DET）通过 XL9555 kXl9555PinSdDet，非直连 GPIO，故传 SDSPI_SLOT_NO_CD。
SdSpiDevConfig sd_spi_dev_cfg(GPIO_NUM_21,       // gpio_cs
                              SDSPI_SLOT_NO_CD,  // gpio_cd（经 XL9555，非直连）
                              SDSPI_SLOT_NO_WP   // gpio_wp
);

SdSpiMountConfig sd_spi_mount_cfg(false,  // format_if_mount_failed
                                  5,      // max_files
                                  0,      // allocation_unit_size（0 = 自动）
                                  false   // disk_status_check_enable
);

// =============================================================================
// 日志实例
// =============================================================================
Logger l_i2c("LoraPager", "I2C", "Bus");
Logger l_spi("LoraPager", "SPI", "Bus");
Logger l_i2s("LoraPager", "I2S", "Bus");
Logger l_xlio("LoraPager", "I2C", "XL9555");
Logger l_disp("LoraPager", "SPI", "Display");
Logger l_ledc("LoraPager", "LEDC");
Logger l_audio("LoraPager", "Audio");
Logger l_lvgl("LoraPager", "LVGL");
Logger l_kb("LoraPager", "I2C", "TCA8418");
Logger l_pmu("LoraPager", "I2C", "BQ25896");
Logger l_enc("LoraPager", "Encoder");
Logger l_kbdrv("LoraPager", "Keyboard");
Logger l_sd("LoraPager", "SPI", "SD");

// =============================================================================
// 设备 / 总线实例（文件作用域，只构造一次）
// =============================================================================
I2cBus i2c_bus(l_i2c);
SpiBus spi_bus(l_spi);
I2sBus i2s_bus(l_i2s);
Xl9555 xl9555(l_xlio);
St7796 display(l_disp);
LedcTimer ledc_timer(l_ledc);
LedcChannel ledc_channel(l_ledc);
AudioCodec audio_codec(l_audio);
LvglPort lvgl_port(l_lvgl);
Tca8418 tca8418(l_kb);
Bq25896 bq25896(l_pmu);
Encoder encoder;  // 旋转编码器（A=GPIO40, B=GPIO41, 按钮=GPIO7）
// 高层键盘驱动（封装 TCA8418，含模式解码）
LilyGoLoRaPagerKeyboard keyboard_driver(l_kbdrv, tca8418);
SdSpi sd_spi(l_sd);  // SD 卡（SPI 模式，CS=GPIO21）

// =============================================================================
// ES8311 编解码器工厂 lambda
// =============================================================================
std::function<esp_err_t()> spk_codec_new_func = []() -> esp_err_t
{
    es8311_codec_cfg_t cfg = {};
    cfg.ctrl_if = audio_codec.GetSpeakerCtrlInterface();
    cfg.gpio_if = audio_codec.GetGpioInterface();
    cfg.codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC;
    cfg.master_mode = false;
    cfg.use_mclk = true;
    cfg.pa_pin = -1;  // 功放通过下面回调中的 XL9555 使能
    cfg.pa_reverted = false;
    cfg.hw_gain = {.pa_voltage = 3.3f, .codec_dac_voltage = 3.3f, .pa_gain = 0.0f};

    const audio_codec_if_t* codec_if = es8311_codec_new(&cfg);
    if (codec_if == nullptr)
        return ESP_ERR_INVALID_STATE;
    audio_codec.SetSpeakerCodecInterface(codec_if);

    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = codec_if,
        .data_if = audio_codec.GetDataInterface(),
    };
    esp_codec_dev_handle_t dev = esp_codec_dev_new(&dev_cfg);
    if (dev == nullptr)
        return ESP_ERR_INVALID_STATE;
    audio_codec.SetSpeakerCodecDeviceHandle(dev);
    return ESP_OK;
};

std::function<esp_err_t()> mic_codec_new_func = []() -> esp_err_t
{
    es8311_codec_cfg_t cfg = {};
    cfg.ctrl_if = audio_codec.GetMicrophoneCtrlInterface();
    cfg.gpio_if = audio_codec.GetGpioInterface();
    cfg.codec_mode = ESP_CODEC_DEV_WORK_MODE_ADC;
    cfg.master_mode = false;
    cfg.use_mclk = true;
    cfg.digital_mic = false;

    const audio_codec_if_t* codec_if = es8311_codec_new(&cfg);
    if (codec_if == nullptr)
        return ESP_ERR_INVALID_STATE;
    audio_codec.SetMicrophoneCodecInterface(codec_if);

    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_IN,
        .codec_if = codec_if,
        .data_if = audio_codec.GetDataInterface(),
    };
    esp_codec_dev_handle_t dev = esp_codec_dev_new(&dev_cfg);
    if (dev == nullptr)
        return ESP_ERR_INVALID_STATE;
    audio_codec.SetMicrophoneCodecDeviceHandle(dev);
    return ESP_OK;
};

// =============================================================================
// LilyGoLoraPager 成员函数实现
// =============================================================================

bool LilyGoLoraPager::InitBootButton()
{
    // 0. 初始化 GPIO0（BOOT 按钮）为输入，低电平触发软件关机
    gpio_config(&boot_button_gpio_cfg);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_NUM_0, boot_button_isr_handler, this);
    return true;
}

bool LilyGoLoraPager::InitCoreBusAndIoExpander()
{
    // 1. I²C 总线
    if (!i2c_bus.Init(i2c_bus_cfg))
    {
        l_i2c.Error("I2C bus init failed");
        return false;
    }
    i2c_bus.Scan();

    // 2. XL9555 IO 扩展器
    //    默认 Init() 将所有输出引脚拉高，从而使能所有外设。
    if (!xl9555.Init(i2c_bus, xl9555_dev_cfg))
    {
        l_xlio.Error("XL9555 init failed");
        return false;
    }

    // 保持 SD_DET 引脚为输入模式；其余引脚为输出（Init 已默认设置）
    xl9555.SetDirection(kXl9555PinSdDet, Xl9555::DIR_INPUT);

    // 3. BQ25896 充电器
    if (!bq25896.Init(i2c_bus, bq25896_dev_cfg))
    {
        l_pmu.Error("BQ25896 init failed");
        return false;
    }
    bq25896.ResetDefault();
    bq25896.SetChargeTargetVoltage(4288);  // 4.288 V
    bq25896.SetChargeCurrent(704);         // 704 mA（≤ 1/2 × 1500 mAh）
    bq25896.EnableMeasure();

    return true;
}

bool LilyGoLoraPager::InitDisplay()
{
    // 1. 共享 SPI 总线
    if (!spi_bus.Init(spi_bus_cfg))
    {
        l_spi.Error("SPI bus init failed");
        return false;
    }

    // 2. 在显示屏初始化前将共享总线 CS 引脚拉高，避免总线冲突
    static const gpio_num_t kCsPins[] = {
        GPIO_NUM_36,  // LoRa CS
        GPIO_NUM_39,  // NFC CS
        GPIO_NUM_21,  // SD CS
    };
    for (auto pin : kCsPins)
    {
        gpio_set_direction(pin, GPIO_MODE_OUTPUT);
        gpio_set_level(pin, 1);
    }

    // 3.（可选）若已连线，通过 XL9555 执行显示屏硬件复位
    //    xl9555.SetLevel(kXl9555PinDispRst, 0);
    //    vTaskDelay(pdMS_TO_TICKS(10));
    //    xl9555.SetLevel(kXl9555PinDispRst, 1);
    //    vTaskDelay(pdMS_TO_TICKS(10));

    // 4. LEDC 背光（初始关闭）
    if (!ledc_timer.Init(ledc_timer_cfg))
    {
        l_ledc.Error("LEDC timer init failed");
        return false;
    }
    if (!ledc_channel.Init(ledc_channel_cfg))
    {
        l_ledc.Error("LEDC channel init failed");
        return false;
    }

    // 5. ST7796 显示面板
    if (!display.Init(spi_bus, spi_display_cfg))
    {
        l_disp.Error("ST7796 display init failed");
        return false;
    }
    // ST7796 为 480×320 控制器；T-LoraPager 使用 480×222 的可视窗口。
    // 行偏移 = 49（可视区域从控制器第 49 行开始）。
    display.SetGap(0, 49);

    // 6. LVGL 移植层
    if (!lvgl_port.Init(lvgl_port_cfg))
    {
        l_lvgl.Error("LVGL port init failed");
        return false;
    }
    if (!lvgl_port.AddDisplay(display, lvgl_display_cfg))
    {
        l_lvgl.Error("LVGL AddDisplay failed");
        return false;
    }

    // 7. 以最大亮度打开背光
    SetDisplayBrightness(100);

    return true;
}

bool LilyGoLoraPager::InitAudio()
{
    // 通过 XL9555 使能功放（AMP_EN）
    xl9555.SetLevel(kXl9555PinAmpEn, 1);

    // 1. I²S 总线
    if (!i2s_bus.Init(i2s_bus_cfg))
    {
        l_i2s.Error("I2S bus init failed");
        return false;
    }
    if (!i2s_bus.ConfigureTxChannel(i2s_chan_cfg))
    {
        l_i2s.Error("I2S TX channel config failed");
        return false;
    }
    if (!i2s_bus.ConfigureRxChannel(i2s_chan_cfg))
    {
        l_i2s.Error("I2S RX channel config failed");
        return false;
    }
    if (!i2s_bus.EnableTxChannel())
    {
        l_i2s.Error("I2S TX enable failed");
        return false;
    }
    if (!i2s_bus.EnableRxChannel())
    {
        l_i2s.Error("I2S RX enable failed");
        return false;
    }

    // 2. AudioCodec（ES8311 — 单芯片同时支持 DAC 和 ADC）
    audio_codec.Init(i2s_bus);

    if (!audio_codec.AddSpeaker(i2c_bus, kEs8311Addr, spk_codec_new_func))
    {
        l_audio.Error("Failed to add ES8311 speaker");
        return false;
    }
    if (!audio_codec.AddMicrophone(i2c_bus, kEs8311Addr, mic_codec_new_func))
    {
        l_audio.Error("Failed to add ES8311 microphone");
        return false;
    }

    return true;
}

bool LilyGoLoraPager::InitKeyboard()
{
    if (!tca8418.Init(i2c_bus, tca8418_dev_cfg, 4, 10))
    {
        l_kb.Error("TCA8418 keyboard init failed");
        return false;
    }
    l_kbdrv.Info("LilyGoLoRaPagerKeyboard driver ready (4x10)");
    return true;
}

bool LilyGoLoraPager::InitEncoder()
{
    // A=GPIO40, B=GPIO41, 按钮=GPIO7（与 SEL_BTN 共用，低电平有效）
    if (!encoder.Init(GPIO_NUM_40, GPIO_NUM_41, GPIO_NUM_7))
    {
        l_enc.Error("Encoder init failed");
        return false;
    }
    if (!lvgl_port.AddEncoder(encoder))
    {
        l_enc.Error("Failed to add encoder to LVGL port");
        return false;
    }
    l_enc.Info("Encoder initialized (A=40, B=41, BTN=7)");
    return true;
}

bool LilyGoLoraPager::InitSdCard()
{
    // 1. 通过 XL9555 使能 SD 电源轨（kXl9555PinSdEn，待原理图确认引脚编号）
    xl9555.SetLevel(kXl9555PinSdEn, 1);
    vTaskDelay(pdMS_TO_TICKS(10));  // 等待电源稳定

    // 2. 在共享 SPI 总线上挂载 SD 卡（CS=GPIO21），VFS 路径 /sdcard
    if (!sd_spi.Init(spi_bus, sd_spi_dev_cfg, sd_spi_mount_cfg, "/sdcard"))
    {
        l_sd.Error("SD card mount failed");
        return false;
    }

    // 3. 打印卡属性
    if (auto info = sd_spi.GetCardInfo())
    {
        l_sd.Info("SD mounted: name=%s  capacity=%.1f MB", info->name.c_str(),
                  static_cast<double>(info->capacity_bytes) / (1024.0 * 1024.0));
    }

    return true;
}

// =============================================================================
// 获取器
// =============================================================================

I2cBus& LilyGoLoraPager::GetI2cBus() { return i2c_bus; }
SpiBus& LilyGoLoraPager::GetSpiBus() { return spi_bus; }
I2sBus& LilyGoLoraPager::GetI2sBus() { return i2s_bus; }
Xl9555& LilyGoLoraPager::GetIoExpander() { return xl9555; }
St7796& LilyGoLoraPager::GetDisplay() { return display; }
LedcTimer& LilyGoLoraPager::GetLedcTimer() { return ledc_timer; }
LedcChannel& LilyGoLoraPager::GetLedcChannel() { return ledc_channel; }
AudioCodec& LilyGoLoraPager::GetAudioCodec() { return audio_codec; }
LvglPort& LilyGoLoraPager::GetLvglPort() { return lvgl_port; }
Tca8418& LilyGoLoraPager::GetKeyboard() { return tca8418; }
Bq25896& LilyGoLoraPager::GetPmu() { return bq25896; }
Encoder& LilyGoLoraPager::GetEncoder() { return encoder; }
LilyGoLoRaPagerKeyboard& LilyGoLoraPager::GetKeyboardDriver() { return keyboard_driver; }
SdSpi& LilyGoLoraPager::GetSdCard() { return sd_spi; }

// =============================================================================
// 辅助方法
// =============================================================================

void LilyGoLoraPager::SetDisplayBrightness(int percent)
{
    if (percent < 0)
        percent = 0;
    if (percent > 100)
        percent = 100;
    // LEDC 10 位：最大占空比 = 1023
    uint32_t duty = static_cast<uint32_t>(1023 * percent / 100);
    if (ledc_channel.SetDutyAndUpdate(duty))
    {
        display_brightness_ = percent;
    }
    else
    {
        l_ledc.Error("Failed to set LEDC duty for brightness");
    }
}

void LilyGoLoraPager::SetDisplayBacklight(bool on) { SetDisplayBrightness(on ? 100 : 0); }

bool LilyGoLoraPager::SetPeripheralPower(uint32_t xl9555_pin, bool enable)
{
    return xl9555.SetLevel(xl9555_pin, enable ? 1 : 0);
}

void LilyGoLoraPager::BootButtonHandler()
{
    if (shutdown_requested.load())
    {
        GetPmu().Shutdown();
        shutdown_requested.store(false);  // 重置标志，避免重复关机
    }
}

void LilyGoLoraPager::KeyboardHandler() { GetKeyboardDriver().Poll(); }

}  // namespace wrapper

#endif  // CONFIG_WRAPPER_ESP32_BOARD_LILYGO_T_LORA_PAGER
