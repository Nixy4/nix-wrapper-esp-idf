#pragma once

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_dev.h"
#include "esp_lcd_panel_ssd1306.h"
#include "local/logger.hpp"
#include <vector>
#include <functional>

namespace wrapper
{

struct SpiBusConfig : public spi_bus_config_t
{
    spi_host_device_t host_id;
    spi_dma_chan_t dma_chan;

    SpiBusConfig(spi_host_device_t host,
                 int mosi,
                 int miso,
                 int sclk,
                 int quadwp,
                 int quadhd,
                 int data4,
                 int data5,
                 int data6,
                 int data7,
                 bool data_default_level,
                 int max_transfer,
                 uint32_t bus_flags,
                 esp_intr_cpu_affinity_t isr_cpu,
                 int intr_flag,
                 spi_dma_chan_t dma)
        : spi_bus_config_t{}
    {
        mosi_io_num = mosi;
        miso_io_num = miso;
        sclk_io_num = sclk;
        quadwp_io_num = quadwp;
        quadhd_io_num = quadhd;
        data4_io_num = data4;
        data5_io_num = data5;
        data6_io_num = data6;
        data7_io_num = data7;
        data_io_default_level = data_default_level;
        max_transfer_sz = max_transfer;
        flags = bus_flags;
        isr_cpu_id = isr_cpu;
        intr_flags = intr_flag;

        host_id = host;
        dma_chan = dma;
    }
};

class SpiBus
{
    Logger& logger_;
    spi_host_device_t host_id_;
    bool initialized_;
    SpiBusConfig config_;

   public:
    SpiBus(Logger& logger);
    ~SpiBus();
    Logger& GetLogger();
    spi_host_device_t GetHostId() const;
    // ops
    bool Init(const SpiBusConfig& config);
    bool Deinit();
    bool Reset();
};

struct SpiDeviceConfig : public spi_device_interface_config_t
{
    SpiDeviceConfig(gpio_num_t cs, int clock_speed_hz, uint8_t mode)
        : spi_device_interface_config_t{}
    {
        command_bits = 0;
        address_bits = 0;
        dummy_bits = 0;
        this->mode = mode;
        duty_cycle_pos = 128;
        cs_ena_pretrans = 0;
        cs_ena_posttrans = 0;
        this->clock_speed_hz = clock_speed_hz;
        input_delay_ns = 0;
        spics_io_num = cs;
        flags = 0;
        queue_size = 3;
        pre_cb = NULL;
        post_cb = NULL;
    }
};

class SpiDevice
{
   protected:
    Logger& logger_;
    spi_device_handle_t dev_handle_;

   public:
    SpiDevice(Logger& logger);
    ~SpiDevice();
    Logger& GetLogger();
    // ops
    bool Init(const SpiBus& bus, const SpiDeviceConfig& config);
    bool Deinit();

    // --- raw pointer variants (inline) ---

    inline bool Transfer(const uint8_t* tx_data, uint8_t* rx_data, size_t len)
    {
        spi_transaction_t t = {};
        t.length = len * 8;
        t.tx_buffer = tx_data;
        t.rx_buffer = rx_data;
        return spi_device_transmit(dev_handle_, &t) == ESP_OK;
    }

    inline bool Write(const uint8_t* data, size_t len)
    {
        spi_transaction_t t = {};
        t.length = len * 8;
        t.tx_buffer = data;
        t.rx_buffer = nullptr;
        return spi_device_transmit(dev_handle_, &t) == ESP_OK;
    }

    inline bool Read(uint8_t* rx_data, size_t len)
    {
        spi_transaction_t t = {};
        t.length = len * 8;
        t.tx_buffer = nullptr;
        t.rx_buffer = rx_data;
        return spi_device_transmit(dev_handle_, &t) == ESP_OK;
    }

    // --- vector variants ---

    bool Transfer(const std::vector<uint8_t>& tx_data, std::vector<uint8_t>& rx_data);

    bool Write(const std::vector<uint8_t>& data);

    bool Read(size_t len, std::vector<uint8_t>& rx_data);

    bool WriteByte(uint8_t data);
    bool ReadByte(uint8_t& data);

    // --- register operations ---
    bool WriteRegBytes(uint8_t reg_addr, const std::vector<uint8_t>& data);
    bool ReadRegBytes(uint8_t reg_addr, std::vector<uint8_t>& data, size_t len);

    bool WriteReg8(uint8_t reg_addr, uint8_t data);
    bool ReadReg8(uint8_t reg_addr, uint8_t& data);
    bool WriteReg16(uint8_t reg_addr, uint16_t data);
    bool ReadReg16(uint8_t reg_addr, uint16_t& data);
    bool WriteReg32(uint8_t reg_addr, uint32_t data);
    bool ReadReg32(uint8_t reg_addr, uint32_t& data);

    bool WriteRegBit(uint8_t reg_addr, uint8_t bit, bool value);
    bool ReadRegBit(uint8_t reg_addr, uint8_t bit, bool& value);
    bool WriteRegBits(uint8_t reg_addr, uint8_t mask, uint8_t value);
    bool ReadRegBits(uint8_t reg_addr, uint8_t mask, uint8_t& value);
};

}  // namespace wrapper