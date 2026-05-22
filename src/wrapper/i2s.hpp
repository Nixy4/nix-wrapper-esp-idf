#pragma once
#include "esp_err.h"
#include "driver/i2s_std.h"
#include "driver/i2s_tdm.h"
#include "driver/i2s_pdm.h"
#include "wrapper/logger.hpp"
#include <vector>

namespace wrapper
{

struct I2sBusConfig : public i2s_chan_config_t
{
    I2sBusConfig(i2s_port_t id, i2s_role_t role, uint32_t dma_desc_num, uint32_t dma_frame_num,
                 bool auto_clear_after_cb, bool auto_clear_before_cb, int intr_priority)
        : i2s_chan_config_t{}  // Zero-initialize base struct using aggregate initialization
    {
        // Set the actual values
        this->id = id;
        this->role = role;
        this->dma_desc_num = dma_desc_num;
        this->dma_frame_num = dma_frame_num;
        this->auto_clear_after_cb = auto_clear_after_cb;
        this->auto_clear_before_cb = auto_clear_before_cb;
        this->intr_priority = intr_priority;
    }
};

struct I2sChanStdConfig : public i2s_std_config_t
{
    I2sChanStdConfig(
        // i2s_std_clk_config_t
        uint32_t sample_rate_hz, i2s_clock_src_t clk_src, uint32_t ext_clk_freq_hz,
        i2s_mclk_multiple_t mclk_multiple, uint32_t bclk_div,
        // i2s_std_slot_config_t
        i2s_data_bit_width_t data_bit_width, i2s_slot_bit_width_t slot_bit_width,
        i2s_slot_mode_t slot_mode, i2s_std_slot_mask_t slot_mask, uint32_t ws_width, bool ws_pol,
        bool bit_shift, bool left_align, bool big_endian, bool bit_order_lsb, gpio_num_t mclk,
        gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, gpio_num_t din, bool mclk_inv = false,
        bool bclk_inv = false, bool ws_inv = false)
        : i2s_std_config_t{}  // Zero-initialize base struct using aggregate initialization
    {
        clk_cfg.sample_rate_hz = sample_rate_hz;
        clk_cfg.clk_src = clk_src;
        clk_cfg.ext_clk_freq_hz = ext_clk_freq_hz;
        clk_cfg.mclk_multiple = mclk_multiple;
        clk_cfg.bclk_div = bclk_div;

        slot_cfg.data_bit_width = data_bit_width;
        slot_cfg.slot_bit_width = slot_bit_width;
        slot_cfg.slot_mode = slot_mode;
        slot_cfg.slot_mask = slot_mask;
        slot_cfg.ws_width = ws_width;
        slot_cfg.ws_pol = ws_pol;
        slot_cfg.bit_shift = bit_shift;
        slot_cfg.left_align = left_align;
        slot_cfg.big_endian = big_endian;
        slot_cfg.bit_order_lsb = bit_order_lsb;

        gpio_cfg.mclk = mclk;
        gpio_cfg.bclk = bclk;
        gpio_cfg.ws = ws;
        gpio_cfg.dout = dout;
        gpio_cfg.din = din;
        gpio_cfg.invert_flags.mclk_inv = mclk_inv;
        gpio_cfg.invert_flags.bclk_inv = bclk_inv;
        gpio_cfg.invert_flags.ws_inv = ws_inv;
    }
};

struct I2SChanTdmConfig : public i2s_tdm_config_t
{
    I2SChanTdmConfig(uint32_t sample_rate_hz, i2s_clock_src_t clk_src, uint32_t ext_clk_freq_hz,
                     i2s_mclk_multiple_t mclk_multiple, uint32_t bclk_div,
                     i2s_data_bit_width_t data_bit_width, i2s_slot_bit_width_t slot_bit_width,
                     i2s_slot_mode_t slot_mode, i2s_tdm_slot_mask_t slot_mask, uint32_t ws_width,
                     bool ws_pol, bool bit_shift, bool left_align, bool big_endian,
                     bool bit_order_lsb, bool skip_mask, uint32_t total_slot, gpio_num_t mclk,
                     gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, gpio_num_t din,
                     bool mclk_inv = false, bool bclk_inv = false, bool ws_inv = false)
        : i2s_tdm_config_t{}  // Zero-initialize base struct using aggregate initialization
    {
        clk_cfg.sample_rate_hz = sample_rate_hz;
        clk_cfg.clk_src = clk_src;
        clk_cfg.ext_clk_freq_hz = ext_clk_freq_hz;
        clk_cfg.mclk_multiple = mclk_multiple;
        clk_cfg.bclk_div = bclk_div;

        slot_cfg.data_bit_width = data_bit_width;
        slot_cfg.slot_bit_width = slot_bit_width;
        slot_cfg.slot_mode = slot_mode;
        slot_cfg.slot_mask = slot_mask;
        slot_cfg.ws_width = ws_width;
        slot_cfg.ws_pol = ws_pol;
        slot_cfg.bit_shift = bit_shift;
        slot_cfg.left_align = left_align;
        slot_cfg.big_endian = big_endian;
        slot_cfg.bit_order_lsb = bit_order_lsb;
        slot_cfg.skip_mask = skip_mask;
        slot_cfg.total_slot = total_slot;

        gpio_cfg.mclk = mclk;
        gpio_cfg.bclk = bclk;
        gpio_cfg.ws = ws;
        gpio_cfg.dout = dout;
        gpio_cfg.din = din;
        gpio_cfg.invert_flags.mclk_inv = mclk_inv;
        gpio_cfg.invert_flags.bclk_inv = bclk_inv;
        gpio_cfg.invert_flags.ws_inv = ws_inv;
    }
};

struct I2sChanPdmRxConfig : public i2s_pdm_rx_config_t
{
    I2sChanPdmRxConfig(uint32_t sample_rate_hz, i2s_clock_src_t clk_src,
                       i2s_mclk_multiple_t mclk_multiple, i2s_pdm_dsr_t dn_sample_mode,
                       uint32_t bclk_div, i2s_data_bit_width_t data_bit_width,
                       i2s_slot_bit_width_t slot_bit_width, i2s_slot_mode_t slot_mode,
                       i2s_pdm_slot_mask_t slot_mask, gpio_num_t clk, gpio_num_t din,
                       bool clk_inv = false)
        : i2s_pdm_rx_config_t{}
    {
        clk_cfg.sample_rate_hz = sample_rate_hz;
        clk_cfg.clk_src = clk_src;
        clk_cfg.mclk_multiple = mclk_multiple;
        clk_cfg.dn_sample_mode = dn_sample_mode;
        clk_cfg.bclk_div = bclk_div;

        slot_cfg.data_bit_width = data_bit_width;
        slot_cfg.slot_bit_width = slot_bit_width;
        slot_cfg.slot_mode = slot_mode;
        slot_cfg.slot_mask = slot_mask;

        gpio_cfg.clk = clk;
        gpio_cfg.din = din;
        gpio_cfg.invert_flags.clk_inv = clk_inv;
    }
};

struct I2sChanPdmTxConfig : public i2s_pdm_tx_config_t
{
    I2sChanPdmTxConfig(uint32_t sample_rate_hz, i2s_clock_src_t clk_src,
                       i2s_mclk_multiple_t mclk_multiple,
                       uint32_t up_sample_fp,  // Changed from i2s_pdm_usr_t up_sample_mode
                       uint32_t bclk_div, i2s_data_bit_width_t data_bit_width,
                       i2s_slot_bit_width_t slot_bit_width, i2s_slot_mode_t slot_mode,
                       // i2s_pdm_slot_mask_t slot_mask, // Removed slot_mask
                       gpio_num_t clk, gpio_num_t dout, bool clk_inv = false)
        : i2s_pdm_tx_config_t{}
    {
        clk_cfg.sample_rate_hz = sample_rate_hz;
        clk_cfg.clk_src = clk_src;
        clk_cfg.mclk_multiple = mclk_multiple;
        clk_cfg.up_sample_fp = up_sample_fp;  // Changed member name
        clk_cfg.bclk_div = bclk_div;

        slot_cfg.data_bit_width = data_bit_width;
        slot_cfg.slot_bit_width = slot_bit_width;
        slot_cfg.slot_mode = slot_mode;
        // slot_cfg.slot_mask = slot_mask; // Removed

        gpio_cfg.clk = clk;
        gpio_cfg.dout = dout;
        gpio_cfg.invert_flags.clk_inv = clk_inv;
    }
};

class I2sBus
{
    Logger& logger_;
    i2s_port_t port_;
    i2s_chan_handle_t tx_chan_handle_ = NULL;
    i2s_chan_handle_t rx_chan_handle_ = NULL;
    uint32_t tx_sample_rate_hz_ = 0;
    uint32_t rx_sample_rate_hz_ = 0;

   public:
    I2sBus(Logger& logger) : logger_(logger) {}
    ~I2sBus();
    bool Init(I2sBusConfig& bus_config);
    bool Deinit();
    bool ConfigureTxChannel(I2sChanStdConfig& chan_config);
    bool ConfigureTxChannel(I2SChanTdmConfig& chan_config);
    bool ConfigureRxChannel(I2SChanTdmConfig& chan_config);
    bool ConfigureRxChannel(I2sChanStdConfig& chan_config);
    bool ConfigureRxChannel(I2sChanPdmRxConfig& chan_config);
    bool ConfigureTxChannel(I2sChanPdmTxConfig& chan_config);

    bool EnableTxChannel();
    bool EnableRxChannel();
    bool DisableTxChannel();
    bool DisableRxChannel();

    bool Write(const void* src, size_t size, size_t& bytes_written, uint32_t timeout_ms = 1000);
    bool Read(void* dest, size_t size, size_t& bytes_read, uint32_t timeout_ms = 1000);

    // C++ style overloads
    bool Write(const std::vector<uint8_t>& data, uint32_t timeout_ms = 1000);
    bool Read(std::vector<uint8_t>& dest, size_t size, uint32_t timeout_ms = 1000);

    i2s_port_t GetPort() const { return port_; }
    i2s_chan_handle_t GetTxHandle() const { return tx_chan_handle_; }
    i2s_chan_handle_t GetRxHandle() const { return rx_chan_handle_; }
    uint32_t GetTxSampleRate() const { return tx_sample_rate_hz_; }
    uint32_t GetRxSampleRate() const { return rx_sample_rate_hz_; }
};

};  // namespace wrapper
