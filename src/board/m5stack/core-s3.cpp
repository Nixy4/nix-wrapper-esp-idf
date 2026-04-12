#include <tuple>

#include "esp_lcd_ili9341.h"
#include "esp_lcd_touch_ft5x06.h"

#include "wrapper/logger.hpp"
#include "wrapper/soc.hpp"
#include "wrapper/i2c.hpp"
#include "wrapper/spi.hpp"
#include "wrapper/i2s.hpp"
#include "wrapper/display.hpp"
#include "wrapper/touch.hpp"
#include "wrapper/lvgl.hpp"
#include "wrapper/audio.hpp"

#include "device/axp2101.hpp"
#include "device/aw9523.hpp"
#include "device/ili9341.hpp"

#include "board/m5stack/core-s3.hpp"

namespace wrapper
{

I2sBusConfig bus_config(I2S_NUM_0, I2S_ROLE_MASTER, 6, 240, true, false, 0);

I2sChanStdConfig tx_config(
    // clk_cfg
    16000,                    // sample_rate_hz
    I2S_CLK_SRC_DEFAULT,      // clk_src
    0,                        // ext_clk_freq_hz
    I2S_MCLK_MULTIPLE_256,    // mclk_multiple
    1,                        // bclk_div
    // slot_cfg
    I2S_DATA_BIT_WIDTH_16BIT, // data_bit_width
    I2S_SLOT_BIT_WIDTH_AUTO,  // slot_bit_width
    I2S_SLOT_MODE_STEREO,     // slot_mode
    I2S_STD_SLOT_BOTH,        // slot_mask
    I2S_DATA_BIT_WIDTH_16BIT, // ws_width
    false,                    // ws_pol
    true,                     // bit_shift
    true,                     // left_align
    false,                    // big_endian
    false,                    // bit_order_lsb
    // gpio_cfg
    GPIO_NUM_0,               // mclk
    GPIO_NUM_34,              // bclk
    GPIO_NUM_33,              // ws
    GPIO_NUM_13,              // dout
    GPIO_NUM_NC,              // din
    false,                    // mclk_inv
    false,                    // bclk_inv
    false);                   // ws_inv

I2SChanTdmConfig rx_config(
    16000,
    I2S_CLK_SRC_DEFAULT,
    0,
    I2S_MCLK_MULTIPLE_256,
    8,
    I2S_DATA_BIT_WIDTH_16BIT,
    I2S_SLOT_BIT_WIDTH_AUTO,
    I2S_SLOT_MODE_STEREO,
    (i2s_tdm_slot_mask_t)(I2S_TDM_SLOT0 | I2S_TDM_SLOT1 | I2S_TDM_SLOT2 | I2S_TDM_SLOT3),
    I2S_TDM_AUTO_WS_WIDTH,
    false,
    true,
    false,
    false,
    false,
    false,
    I2S_TDM_AUTO_SLOT_NUM,
    GPIO_NUM_0,
    GPIO_NUM_34,
    GPIO_NUM_33,
    GPIO_NUM_NC,
    GPIO_NUM_NC,
    false,
    false,
    false);

I2cBusConfig i2c_bus1_config(
    I2C_NUM_1,
    GPIO_NUM_12,
    GPIO_NUM_11,
    I2C_CLK_SRC_DEFAULT,
    7,
    0,
    0,
    true,
    false);

SpiBusConfig spi_bus_config(
    SPI3_HOST,                      // host
    GPIO_NUM_37,                    // mosi
    GPIO_NUM_NC,                    // miso
    GPIO_NUM_36,                    // sclk
    GPIO_NUM_NC,                    // quadwp
    GPIO_NUM_NC,                    // quadhd
    GPIO_NUM_NC,                    // data4
    GPIO_NUM_NC,                    // data5
    GPIO_NUM_NC,                    // data6
    GPIO_NUM_NC,                    // data7
    false,                          // data_default_level
    320 * 240 * sizeof(uint16_t),   // max_transfer
    SPICOMMON_BUSFLAG_MASTER,       // bus_flags
    ESP_INTR_CPU_AFFINITY_AUTO,     // isr_cpu
    0,                              // intr_flags
    SPI_DMA_CH_AUTO);               // dma

I2cTouchConfig ft5x06_config(
    // io_config parameters
    0x38,                   // dev_addr
    nullptr,                // on_color_trans_done
    nullptr,                // user_ctx
    1,                      // control_phase_bytes
    6,                      // dc_bit_offset
    8,                      // lcd_cmd_bits
    8,                      // lcd_param_bits
    0,                      // dc_low_on_data
    0,                      // disable_control_phase
    400000,                 // scl_speed_hz (400kHz)
    // touch_config parameters
    320,                    // x_max
    240,                    // y_max
    GPIO_NUM_NC,            // rst_gpio_num
    GPIO_NUM_NC,            // int_gpio_num
    0,                      // levels.reset
    0,                      // levels.interrupt
    0,                      // flags.swap_xy
    0,                      // flags.mirror_x
    0,                      // flags.mirror_y
    nullptr,                // process_coordinates
    nullptr                 // interrupt_callback
);

SpiDisplayConfig spi_lcd_config(
    // io_config parameters
    GPIO_NUM_3,                // cs_gpio
    GPIO_NUM_35,               // dc_gpio
    2,                         // spi_mode
    40 * 1000 * 1000,          // clock_speed_hz (40MHz)
    8,                         // lcd_cmd_bits
    8,                         // lcd_param_bits
    10,                        // trans_queue_depth
    nullptr,                   // on_color_trans_done
    nullptr,                   // user_ctx
    0,                         // cs_ena_pretrans
    0,                         // cs_ena_posttrans
    // io_config flags
    0,                         // dc_high_on_cmd
    0,                         // dc_low_on_data
    0,                         // dc_low_on_param
    0,                         // octal_mode
    0,                         // quad_mode
    0,                         // sio_mode
    0,                         // lsb_first
    0,                         // cs_high_active
    // panel_config parameters
    GPIO_NUM_NC,               // reset_gpio
    LCD_RGB_ELEMENT_ORDER_BGR, // rgb_order
    LCD_RGB_DATA_ENDIAN_BIG,   // data_endian
    16,                        // bits_per_pixel
    false,                     // reset_active_high
    nullptr                    // vendor_conf
);

I2cDeviceConfig axp2101_config(Axp2101::DEFAULT_ADDR, Axp2101::DEFAULT_SPEED);
I2cDeviceConfig aw9523_config(Aw9523::DEFAULT_ADDR, Aw9523::DEFAULT_SPEED);

LvglPortConfig lvgl_port_config(
    5,                                   // task_priority
    8192,                                // task_stack
    1,                                   // task_affinity
    500,                                 // task_max_sleep_ms
    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT, // task_stack_caps
    20                                   // timer_period_ms
);

LvglDisplayConfig lvgl_display_config(
    320 * 240,              // buffer_size
    true,                   // double_buffer
    0,                      // trans_size
    320,                    // hres
    240,                    // vres
    false,                  // monochrome
    false,                  // swap_xy
    false,                  // mirror_x
    false,                  // mirror_y
    LV_COLOR_FORMAT_RGB565, // color_format
    true,                   // buff_dma
    true,                   // buff_spiram
    false,                  // sw_rotate
    true,                   // swap_bytes
    false,                  // full_refresh
    false                   // direct_mode
);

LvglTouchConfig lvgl_touch_config(0.0f, 0.0f);

Logger logger_i2c_bus1("M5StackCoreS3", "I2C", "Bus");
Logger logger_spi_bus("M5StackCoreS3", "SPI", "Bus");
Logger logger_i2s_bus("M5StackCoreS3", "I2S", "Bus");

Logger logger_axp2101("M5StackCoreS3", "I2C", "PowerManager");
Logger logger_aw9523("M5StackCoreS3", "I2C", "GpioExpander");
Logger logger_ft5x06("M5StackCoreS3", "I2C", "Touch");
Logger logger_ili9341("M5StackCoreS3", "SPI", "Display");
Logger logger_lvgl("M5StackCoreS3", "LVGL", "Port");
Logger logger_audio_codec("M5StackCoreS3", "Audio", "Codec");

I2cBus i2c_bus1(logger_i2c_bus1);
SpiBus spi_bus(logger_spi_bus);
I2sBus i2s_bus(logger_i2s_bus);

Axp2101 axp2101(logger_axp2101);
Aw9523 aw9523(logger_aw9523);
I2cTouch ft5x06(logger_ft5x06);
Ili9341 ili9341(logger_ili9341);
LvglPort lvgl_port(logger_lvgl);

AudioCodec audio_codec(logger_audio_codec);
std::function<esp_err_t()> spk_codec_new_func = []() -> esp_err_t
{
  aw88298_codec_cfg_t aw88298_cfg = {
      .ctrl_if = audio_codec.GetSpeakerCtrlInterface(),
      .gpio_if = audio_codec.GetGpioInterface(),
      .reset_pin = GPIO_NUM_NC,
      .hw_gain = {
          .pa_voltage = 5.0f,
          .codec_dac_voltage = 3.3f,
          .pa_gain = 1.f},
  };
  const audio_codec_if_t *codec_if = aw88298_codec_new(&aw88298_cfg);
  if (codec_if == NULL)
  {
    return ESP_ERR_INVALID_STATE;
  }
  audio_codec.SetSpeakerCodecInterface(codec_if);

  esp_codec_dev_handle_t codec_dev_handle = nullptr;
  esp_codec_dev_cfg_t codec_dev_cfg = {
      .dev_type = ESP_CODEC_DEV_TYPE_OUT,
      .codec_if = codec_if,
      .data_if = audio_codec.GetDataInterface(),
  };
  codec_dev_handle = esp_codec_dev_new(&codec_dev_cfg);
  if (codec_dev_handle == nullptr)
  {
    return ESP_ERR_INVALID_STATE;
  }
  audio_codec.SetSpeakerCodecDeviceHandle(codec_dev_handle);
  return ESP_OK;
};
std::function<esp_err_t()> mic_codec_new_func = []() -> esp_err_t
{
  es7210_codec_cfg_t codec_cfg = {
      .ctrl_if = audio_codec.GetMicrophoneCtrlInterface(),
      .master_mode = false,
      .mic_selected = 0,
      .mclk_src = ES7210_MCLK_FROM_PAD,
      .mclk_div = 0,
  };
  const audio_codec_if_t *codec_if = es7210_codec_new(&codec_cfg);
  if (codec_if == NULL)
  {
    return ESP_ERR_INVALID_STATE;
  }
  audio_codec.SetMicrophoneCodecInterface(codec_if);
  esp_codec_dev_handle_t codec_dev_handle = nullptr;
  esp_codec_dev_cfg_t codec_dev_cfg = {
      .dev_type = ESP_CODEC_DEV_TYPE_IN,
      .codec_if = codec_if,
      .data_if = audio_codec.GetDataInterface(),
  };
  codec_dev_handle = esp_codec_dev_new(&codec_dev_cfg);
  if (codec_dev_handle == nullptr)
  {
    return ESP_ERR_INVALID_STATE;
  }
  audio_codec.SetMicrophoneCodecDeviceHandle(codec_dev_handle);
  return ESP_OK;
};

bool M5StackCoreS3::InitBus(bool i2c1, bool spi, bool i2s)
{
  if (i2c1) {
      if (!i2c_bus1.Init(i2c_bus1_config)) return false;
  }
  
  if (spi) {
      if (!spi_bus.Init(spi_bus_config)) {
          spi_bus.GetLogger().Error("Failed to initialize SPI bus");
          return false;
      }
  }
  
  if (i2s) {
      if (!i2s_bus.Init(bus_config)) return false;
      if (!i2s_bus.ConfigureTxChannel(tx_config)) return false;
      if (!i2s_bus.ConfigureRxChannel(rx_config)) return false;
  }
  return true;
}

bool M5StackCoreS3::InitDevice(bool power, bool audio, bool display, bool touch)
{ 
  if (power) {
      // AXP2101
      if (!axp2101.Init(i2c_bus1, axp2101_config)) return false;
      
      uint8_t data = 0x00;
      if (!axp2101.ReadReg8(0x90, data, -1)) return false;
      data |= 0b10110100;
      std::tuple<uint8_t, uint8_t> axp_cmds[] = {
          {0x90, data},
          {0x99, (uint8_t)(0b11110 - 5)},
          {0x97, (uint8_t)(0b11110 - 2)},
          {0x69, 0b00110101},
          {0x30, 0b111111},
          {0x90, 0xBF},
          {0x94, 33 - 5},
          {0x95, 33 - 5},
      };
      for (const auto &[reg, value] : axp_cmds)
      {
        if (!axp2101.WriteReg8(reg, value, -1)) return false;
      }
      axp2101.GetLogger().Info("Configured successfully");

      // AW9523
      if (!aw9523.Init(i2c_bus1, aw9523_config)) return false;
      
      std::tuple<uint8_t, uint8_t> aw_cmds[] = {
          {0x02, 0b00000111},
          {0x03, 0b10001111},
          {0x04, 0b00011000},
          {0x05, 0b00001100},
          {0x11, 0b00010000},
          {0x12, 0b11111111},
          {0x13, 0b11111111},
      };
      for (const auto &[reg, value] : aw_cmds)
      {
        if (!aw9523.WriteReg8(reg, value, -1))
        {
          aw9523.GetLogger().Error("Failed to write register 0x%02X", reg);
          return false;
        }
      }
      aw9523.GetLogger().Info("Configured successfully");
  }

  if (touch) {
      if (!ft5x06.Init(i2c_bus1, ft5x06_config, esp_lcd_touch_new_i2c_ft5x06)) return false;
  }

  if (display) {
      if (!ili9341.Init(spi_bus, spi_lcd_config))
      {
        ili9341.GetLogger().Error("Failed to initialize display");
        return false;
      }
  }

  if (audio) {
      audio_codec.Init(i2s_bus);
      // speaker
      if (!audio_codec.AddSpeaker(i2c_bus1, AW88298_CODEC_DEFAULT_ADDR, spk_codec_new_func))
      {
        audio_codec.GetLogger().Error("Failed to add speaker");
        return false;
      }
      // microphone
      if (!audio_codec.AddMicrophone(i2c_bus1, ES7210_CODEC_DEFAULT_ADDR, mic_codec_new_func))
      {
        audio_codec.GetLogger().Error("Failed to add microphone");
        return false;
      }
  }
  
  return true;
}

bool M5StackCoreS3::InitMiddleware(bool lvgl)
{
    if (lvgl) {
      if (!lvgl_port.Init(lvgl_port_config)) {
        return false;
      }
      
      if (!lvgl_port.AddDisplay(ili9341, lvgl_display_config)) {
        return false;
      }
      
      if (!lvgl_port.AddTouch(ft5x06, lvgl_touch_config)) {
        return false;
      }
    }
    return true;
}

bool M5StackCoreS3::Init()
{
    if (initialized_) return true;
    if (!InitBus(true, true, true)) return false;
    if (!InitDevice(true, true, true, true)) return false;
    if (!InitMiddleware(true)) return false;
    
    initialized_ = true;
    return true;
}

I2cBus& M5StackCoreS3::GetI2cBus1()
{
    return i2c_bus1;
}

SpiBus& M5StackCoreS3::GetSpiBus()
{
  return spi_bus;
}

I2sBus& M5StackCoreS3::GetI2sBus()
{
  return i2s_bus;
}

Axp2101& M5StackCoreS3::GetPowerManagement()
{
  return axp2101;
}

Aw9523& M5StackCoreS3::GetGpioExpander()
{
  return aw9523;
}

LvglPort& M5StackCoreS3::GetLvglPort()
{
  return lvgl_port;
}

AudioCodec& M5StackCoreS3::GetAudioCodec()
{
  return audio_codec;
}

} // namespace wrapper