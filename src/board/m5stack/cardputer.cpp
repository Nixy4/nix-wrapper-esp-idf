#include "sdkconfig.h"
#include "cardputer.hpp"

#if CONFIG_WRAPPER_ESP32_BOARD_M5STACK_CARDPUTER

#include "esp_lcd_st7789.h"
#include <cmath>
#include "local/logger.hpp"
#include "local/i2c.hpp"
#include "local/spi.hpp"
#include "local/display.hpp"
#include "local/i2s.hpp"
#include "registry/audio.hpp"
#include "device/m5stack_cardputer_keyboard.hpp"

namespace wrapper
{

// SPI Bus Config
SpiBusConfig spi_bus_config(SPI2_HOST,
                            GPIO_NUM_35,  // mosi
                            GPIO_NUM_NC,  // miso
                            GPIO_NUM_36,  // sclk
                            -1,
                            -1,
                            -1,
                            -1,
                            -1,
                            -1,    // quad/data pins
                            true,  // data_default_level
                            4096,  // max_transfer_sz
                            SPICOMMON_BUSFLAG_MASTER,
                            ESP_INTR_CPU_AFFINITY_AUTO,
                            0,
                            SPI_DMA_CH_AUTO);

// SPI Display Config
SpiDisplayConfig spi_display_config(GPIO_NUM_37,  // cs
                                    GPIO_NUM_34,  // dc
                                    0,            // spi_mode
                                    40000000,     // clock_speed_hz
                                    8,            // cmd_bits
                                    8,            // param_bits
                                    10,           // queue_depth
                                    nullptr,      // on_color_trans_done
                                    nullptr,      // user_ctx
                                    0,
                                    0,  // cs_ena pre/post
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,           // flags
                                    GPIO_NUM_33  // reset_gpio
);

// I2C Config (Grove)
I2cBusConfig i2c_bus_config(I2C_NUM_1,
                            GPIO_NUM_2,  // sda
                            GPIO_NUM_1,  // scl
                            I2C_CLK_SRC_DEFAULT,
                            7,     // glitch_ignore
                            0,     // intr_prio
                            0,     // queue_depth
                            true,  // enable_pullup
                            false  // enable_pd
);

// I2S Config (Speaker - NS4168)
// Note: Speaker uses BCLK=41, SDATA=42, LRCLK=43
I2sBusConfig i2s_bus_cfg(I2S_NUM_0,
                         I2S_ROLE_MASTER,
                         6,      // dma_desc_num
                         1024,   // dma_frame_num
                         true,   // auto_clear_after_cb
                         false,  // auto_clear_before_cb
                         0       // intr_priority
);

I2sChanStdConfig i2s_speaker_chan_cfg(
    48000,                     // sample_rate_hz
    I2S_CLK_SRC_DEFAULT,       // clk_src
    0,                         // ext_clk_freq_hz
    I2S_MCLK_MULTIPLE_256,     // mclk_multiple
    8,                         // bclk_div
    I2S_DATA_BIT_WIDTH_16BIT,  // data_bit_width
    I2S_SLOT_BIT_WIDTH_AUTO,   // slot_bit_width
    I2S_SLOT_MODE_MONO,  // slot_mode (Speaker usually mono or stereo, NS4168 is mono amp but input
                         // usually stereo I2S?)
    I2S_STD_SLOT_BOTH,   // slot_mask
    16,                  // ws_width
    false,               // ws_pol
    false,               // bit_shift
    false,               // left_align
    false,               // big_endian
    false,               // bit_order_lsb
    GPIO_NUM_NC,         // mclk
    GPIO_NUM_41,         // bclk
    GPIO_NUM_43,         // ws (LRCLK)
    GPIO_NUM_42,         // dout (SDATA)
    GPIO_NUM_NC          // din
);

I2sChanPdmRxConfig i2s_mic_chan_cfg(
    16000,                  // sample_rate_hz
    I2S_CLK_SRC_DEFAULT,    // clk_src
    I2S_MCLK_MULTIPLE_256,  // mclk_multiple
    I2S_PDM_DSR_8S,  // dn_sample_mode (Downsampling rate: 8 for PDM usually implies 1/8th, verify
                     // with driver default or trial)
    8,  // bclk_div (calculated automatically if 0? Check driver docs, usually 8 is standard for
        // PDM)
    I2S_DATA_BIT_WIDTH_16BIT,  // data_bit_width
    I2S_SLOT_BIT_WIDTH_AUTO,   // slot_bit_width
    I2S_SLOT_MODE_MONO,        // slot_mode
    I2S_PDM_SLOT_BOTH,  // slot_mask (Use BOTH or LEFT/RIGHT? Info2 doesn't specify, but BOTH is
                        // safe for mono PDM usually)
    GPIO_NUM_43,        // clk
    GPIO_NUM_46,        // din
    false               // clk_inv
);

/*
Keyboard & Battery (No wrapper found for GPIO/ADC yet)
Keyboard: Matrix (Rows: G13,15,3,4,5,6,7; Cols: G8,9,11)
*/

/*
Battery: ADC G10
*/
Keyboard keyboard;

/*
SD Card (No wrapper found yet)
Pins: CS=12, MOSI=14, CLK=40, MISO=39
*/

// Loggers
Logger logger_i2c("I2C");

Logger logger_display("Display");
Logger logger_spi("SPI");

Logger logger_i2s("I2S");
Logger logger_spk("Speaker");
Logger logger_mic("Mic");

I2cBus i2c_bus(logger_i2c);

SpiBus spi_bus(logger_spi);
SpiDisplay display(logger_display, spi_bus);

I2sBus i2s_bus(logger_i2s);
Speaker speaker(logger_spk);
Microphone mic(logger_mic);

bool M5StackCardputer::Init()
{
    i2c_bus.Init(i2c_bus_config);                                // pass
    spi_bus.Init(spi_bus_config);                                // pass
    display.Init(spi_display_config, esp_lcd_new_panel_st7789);  // pass
    i2s_bus.Init(i2s_bus_cfg);                                   // pass
    i2s_bus.ConfigureTxChannel(i2s_speaker_chan_cfg);            // pass
    i2s_bus.ConfigureRxChannel(i2s_mic_chan_cfg);                // pass

    // 5. Keyboard Init (Cardputer Matrix)
    KeyboardConfig keyboard_config;
    keyboard_config.input_pins = {13, 15, 3, 4, 5, 6, 7};
    keyboard_config.output_pins = {8, 9, 11};
    keyboard.Init(keyboard_config);
    logger_display.Info("Keyboard Initialized");
    return true;
}

}  // namespace wrapper

#endif