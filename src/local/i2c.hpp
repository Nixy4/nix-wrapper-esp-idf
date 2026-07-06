#pragma once
#include <vector>
#include "driver/i2c_master.h"
#include "local/logger.hpp"

namespace wrapper
{

struct I2cBusConfig : public i2c_master_bus_config_t
{
   public:
    I2cBusConfig(i2c_port_t port,
                 gpio_num_t sda,
                 gpio_num_t scl,
                 i2c_clock_source_t clk_src,
                 uint8_t glitch_ignore,
                 int intr_prio,
                 size_t queue_depth,
                 bool enable_pullup,
                 bool enable_pd)
        : i2c_master_bus_config_t{}
    {
        i2c_port = (i2c_port_num_t)port;
        sda_io_num = sda;
        scl_io_num = scl;
        clk_source = clk_src;
        glitch_ignore_cnt = glitch_ignore;
        intr_priority = intr_prio;
        trans_queue_depth = queue_depth;
        flags.enable_internal_pullup = enable_pullup;
        flags.allow_pd = enable_pd;
    }
};

class I2cBus
{
    Logger& logger_;
    i2c_port_t port_;
    i2c_master_bus_handle_t bus_handle_;

    esp_err_t ProbeInternal(int addr);

   public:
    I2cBus(Logger& logger);
    ~I2cBus();

    Logger& GetLogger();
    i2c_port_t GetPort() const;
    i2c_master_bus_handle_t GetHandle() const;

    // operations
    bool Init(const I2cBusConfig& config);
    bool Deinit();
    bool Reset();
    bool Probe(int addr);

    template <typename... Args>
    bool Scan(Args... args)
    {
        std::vector<uint8_t> addrs = {static_cast<uint8_t>(args)...};
        return Scan(addrs);
    }

    bool Scan(const std::vector<uint8_t>& addrs);
    bool Scan(int start_addr, int end_addr);
    bool Scan();
};

struct I2cDeviceConfig : public i2c_device_config_t
{
    I2cDeviceConfig(uint8_t addr, uint32_t speed_hz) : i2c_device_config_t{}
    {
        dev_addr_length = I2C_ADDR_BIT_LEN_7;
        device_address = addr;
        scl_speed_hz = speed_hz;
        scl_wait_us = 0;
        flags.disable_ack_check = 0;
    }
};

class I2cDevice
{
   protected:
    Logger& logger_;
    i2c_master_dev_handle_t dev_handle_;

   public:
    I2cDevice(Logger& logger);
    ~I2cDevice();
    Logger& GetLogger();
    bool Init(const I2cBus& bus, const I2cDeviceConfig& config);
    bool Deinit();

    inline bool WriteBytes(const uint8_t* data, size_t length, int timeout_ms)
    {
        return i2c_master_transmit(dev_handle_, data, length, timeout_ms) == ESP_OK;
    }

    inline bool ReadBytes(uint8_t* data, size_t length, int timeout_ms)
    {
        return i2c_master_receive(dev_handle_, data, length, timeout_ms) == ESP_OK;
    }

    inline bool WriteReadBytes(const uint8_t* write_data,
                               size_t write_length,
                               uint8_t* read_data,
                               size_t read_length,
                               int timeout_ms)
    {
        return i2c_master_transmit_receive(dev_handle_, write_data, write_length, read_data,
                                           read_length, timeout_ms) == ESP_OK;
    }

    bool WriteByte(uint8_t data, int timeout_ms);
    bool ReadByte(uint8_t& data, int timeout_ms);

    bool WriteBytes(const std::vector<uint8_t>& data, int timeout_ms);
    bool ReadBytes(std::vector<uint8_t>& data, size_t len, int timeout_ms);
    bool WriteReadBytes(const std::vector<uint8_t>& write_data,
                        std::vector<uint8_t>& read_data,
                        size_t read_len,
                        int timeout_ms);

    bool WriteRegBytes(uint8_t reg_addr, const std::vector<uint8_t>& data, int timeout_ms);
    bool ReadRegBytes(uint8_t reg_addr, std::vector<uint8_t>& data, size_t len, int timeout_ms);

    bool WriteReg8(uint8_t reg_addr, uint8_t data, int timeout_ms);
    bool ReadReg8(uint8_t reg_addr, uint8_t& data, int timeout_ms);
    bool WriteReg16(uint8_t reg_addr, uint16_t data, int timeout_ms);
    bool ReadReg16(uint8_t reg_addr, uint16_t& data, int timeout_ms);
    bool WriteReg32(uint8_t reg_addr, uint32_t data, int timeout_ms);
    bool ReadReg32(uint8_t reg_addr, uint32_t& data, int timeout_ms);

    bool WriteRegBit(uint8_t reg_addr, uint8_t bit, bool value, int timeout_ms);
    bool ReadRegBit(uint8_t reg_addr, uint8_t bit, bool& value, int timeout_ms);
    bool WriteRegBits(uint8_t reg_addr, uint8_t mask, uint8_t value, int timeout_ms);
    bool ReadRegBits(uint8_t reg_addr, uint8_t mask, uint8_t& value, int timeout_ms);
};

}  // namespace wrapper
