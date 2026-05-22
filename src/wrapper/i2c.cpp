#include "wrapper/i2c.hpp"
#include <iomanip>
#include <sstream>
#include <cstring>
#include <vector>

// --- I2cBus ---
using namespace wrapper;

I2cBus::I2cBus(Logger& logger) : logger_(logger), bus_handle_(nullptr) {}

I2cBus::~I2cBus() { Deinit(); }

Logger& I2cBus::GetLogger() { return logger_; }

i2c_master_bus_handle_t I2cBus::GetHandle() const { return bus_handle_; }

i2c_port_t I2cBus::GetPort() const { return port_; }

bool I2cBus::Init(const I2cBusConfig& config)
{
    if (bus_handle_ != nullptr)
    {
        logger_.Warning("Already initialized. Deinitializing first.");
        Deinit();
    }

    esp_err_t ret = i2c_new_master_bus(&config, &bus_handle_);
    if (ret == ESP_OK)
    {
        logger_.Info("Initialized (Port: %d, SDA: %d, SCL: %d)", config.i2c_port, config.sda_io_num,
                     config.scl_io_num);
        port_ = (i2c_port_t)config.i2c_port;
        return true;
    }
    else
    {
        logger_.Error("Failed to initialize: %s", esp_err_to_name(ret));
        return false;
    }
}

bool I2cBus::Deinit()
{
    if (bus_handle_ != nullptr)
    {
        esp_err_t ret = i2c_del_master_bus(bus_handle_);
        if (ret == ESP_OK)
        {
            logger_.Info("Deinitialized");
            bus_handle_ = nullptr;
            return true;
        }
        else
        {
            logger_.Error("Failed to deinitialize: %s", esp_err_to_name(ret));
            return false;
        }
    }
    return true;
}

bool I2cBus::Reset()
{
    if (bus_handle_ == nullptr)
    {
        logger_.Error("Cannot reset: Not initialized");
        return false;
    }

    esp_err_t ret = i2c_master_bus_reset(bus_handle_);
    if (ret == ESP_OK)
    {
        logger_.Info("Reset successfully");
        return true;
    }
    else
    {
        logger_.Error("Failed to reset: %s", esp_err_to_name(ret));
        return false;
    }
}

bool I2cBus::Probe(int addr) { return ProbeInternal(addr) == ESP_OK; }

esp_err_t I2cBus::ProbeInternal(int addr)
{
    if (bus_handle_ == nullptr)
    {
        return ESP_ERR_INVALID_STATE;
    }

    // Default timeout 50ms for probing
    return i2c_master_probe(bus_handle_, static_cast<uint16_t>(addr), 50);
}

bool I2cBus::Scan(const std::vector<uint8_t>& addrs)
{
    esp_err_t last_err = ESP_OK;
    bool found_any = false;
    for (uint8_t addr : addrs)
    {
        esp_err_t ret = ProbeInternal(addr);
        if (ret == ESP_OK)
        {
            found_any = true;
            logger_.Info("Found device at address 0x%02X", addr);
        }
        else if (ret != ESP_ERR_NOT_FOUND)
        {
            last_err = ret;
            logger_.Error("Failed to probe address 0x%02X: %s", addr, esp_err_to_name(ret));
        }
    }
    return found_any && (last_err == ESP_OK);
}

bool I2cBus::Scan(int start_addr, int end_addr)
{
    if (start_addr > end_addr)
    {
        logger_.Error("Invalid scan range: 0x%02X - 0x%02X", start_addr, end_addr);
        return false;
    }

    if (bus_handle_ == nullptr)
    {
        logger_.Error("Cannot scan: Not initialized");
        return false;
    }

    logger_.Info("Scanning from 0x%02X to 0x%02X...", start_addr, end_addr);

    int found_count = 0;

    logger_.Info("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f");

    for (int i = 0; i < 128; i += 16)
    {
        if (i + 15 < start_addr || i > end_addr)
        {
            continue;
        }

        std::stringstream ss;
        ss << std::hex << std::setw(2) << std::setfill('0') << i << ":";

        for (int j = 0; j < 16; j++)
        {
            int addr = i + j;
            if (addr < start_addr || addr > end_addr)
            {
                ss << "   ";
            }
            else
            {
                esp_err_t ret = ProbeInternal(addr);
                if (ret == ESP_OK)
                {
                    ss << " " << std::hex << std::setw(2) << std::setfill('0') << addr;
                    found_count++;
                }
                else if (ret == ESP_ERR_TIMEOUT)
                {
                    ss << " UU";
                }
                else
                {
                    ss << " --";
                }
            }
        }
        logger_.Info("%s", ss.str().c_str());
    }

    logger_.Info("Scan complete. Found %d devices.", found_count);
    return true;  // Always return true if scan completed without fatal bus error
}

bool I2cBus::Scan() { return Scan(0x00, 0x7F); }

// --- I2cDevice ---

I2cDevice::I2cDevice(Logger& logger) : logger_(logger), dev_handle_(nullptr) {}

I2cDevice::~I2cDevice() { Deinit(); }

Logger& I2cDevice::GetLogger() { return logger_; }

bool I2cDevice::Init(const I2cBus& bus, const I2cDeviceConfig& config)
{
    if (dev_handle_ != nullptr)
    {
        logger_.Warning("Device already initialized. Deinitializing first.");
        Deinit();
    }

    if (bus.GetHandle() == nullptr)
    {
        logger_.Error("Bus not initialized");
        return false;
    }

    esp_err_t ret = i2c_master_bus_add_device(bus.GetHandle(), &config, &dev_handle_);
    if (ret == ESP_OK)
    {
        logger_.Info("Device initialized (Addr: 0x%02X)", config.device_address);
        return true;
    }
    else
    {
        logger_.Error("Failed to add device: %s", esp_err_to_name(ret));
        return false;
    }
}

bool I2cDevice::Deinit()
{
    if (dev_handle_ == nullptr)
    {
        return true;
    }
    esp_err_t ret = i2c_master_bus_rm_device(dev_handle_);
    if (ret == ESP_OK)
    {
        logger_.Info("Device deinitialized");
        dev_handle_ = nullptr;
        return true;
    }
    else
    {
        logger_.Error("Failed to remove device: %s", esp_err_to_name(ret));
        return false;
    }
}

bool I2cDevice::WriteBytes(const std::vector<uint8_t>& data, int timeout_ms)
{
    if (dev_handle_ == nullptr)
        return false;
    return i2c_master_transmit(dev_handle_, data.data(), data.size(), timeout_ms) == ESP_OK;
}

bool I2cDevice::ReadBytes(std::vector<uint8_t>& data, size_t len, int timeout_ms)
{
    if (dev_handle_ == nullptr)
        return false;
    data.resize(len);
    return i2c_master_receive(dev_handle_, data.data(), len, timeout_ms) == ESP_OK;
}

bool I2cDevice::WriteReadBytes(const std::vector<uint8_t>& write_data,
                               std::vector<uint8_t>& read_data, size_t read_len, int timeout_ms)
{
    if (dev_handle_ == nullptr)
        return false;
    read_data.resize(read_len);
    return i2c_master_transmit_receive(dev_handle_, write_data.data(), write_data.size(),
                                       read_data.data(), read_len, timeout_ms) == ESP_OK;
}

bool I2cDevice::WriteByte(uint8_t data, int timeout_ms)
{
    if (dev_handle_ == nullptr)
        return false;
    return i2c_master_transmit(dev_handle_, &data, 1, timeout_ms) == ESP_OK;
}

bool I2cDevice::ReadByte(uint8_t& data, int timeout_ms)
{
    if (dev_handle_ == nullptr)
        return false;
    return i2c_master_receive(dev_handle_, &data, 1, timeout_ms) == ESP_OK;
}

bool I2cDevice::WriteRegBytes(uint8_t reg_addr, const std::vector<uint8_t>& data, int timeout_ms)
{
    if (dev_handle_ == nullptr)
        return false;

    // Optimization: Use stack for small data
    size_t total_len = 1 + data.size();
    if (total_len <= 128)
    {
        uint8_t buffer[128];
        buffer[0] = reg_addr;
        if (!data.empty())
        {
            memcpy(buffer + 1, data.data(), data.size());
        }
        return i2c_master_transmit(dev_handle_, buffer, total_len, timeout_ms) == ESP_OK;
    }
    else
    {
        std::vector<uint8_t> buffer;
        buffer.reserve(total_len);
        buffer.push_back(reg_addr);
        buffer.insert(buffer.end(), data.begin(), data.end());
        return i2c_master_transmit(dev_handle_, buffer.data(), buffer.size(), timeout_ms) == ESP_OK;
    }
}

bool I2cDevice::ReadRegBytes(uint8_t reg_addr, std::vector<uint8_t>& data, size_t len,
                             int timeout_ms)
{
    if (dev_handle_ == nullptr)
        return false;
    data.resize(len);
    return i2c_master_transmit_receive(dev_handle_, &reg_addr, 1, data.data(), len, timeout_ms) ==
           ESP_OK;
}

bool I2cDevice::WriteReg8(uint8_t reg_addr, uint8_t data, int timeout_ms)
{
    if (dev_handle_ == nullptr)
        return false;
    uint8_t buffer[2] = {reg_addr, data};
    return i2c_master_transmit(dev_handle_, buffer, 2, timeout_ms) == ESP_OK;
}

bool I2cDevice::ReadReg8(uint8_t reg_addr, uint8_t& data, int timeout_ms)
{
    if (dev_handle_ == nullptr)
        return false;
    return i2c_master_transmit_receive(dev_handle_, &reg_addr, 1, &data, 1, timeout_ms) == ESP_OK;
}

bool I2cDevice::WriteReg16(uint8_t reg_addr, uint16_t data, int timeout_ms)
{
    if (dev_handle_ == nullptr)
        return false;
    // Big Endian
    uint8_t buffer[3] = {reg_addr, (uint8_t)(data >> 8), (uint8_t)(data & 0xFF)};
    return i2c_master_transmit(dev_handle_, buffer, 3, timeout_ms) == ESP_OK;
}

bool I2cDevice::ReadReg16(uint8_t reg_addr, uint16_t& data, int timeout_ms)
{
    if (dev_handle_ == nullptr)
        return false;
    uint8_t buffer[2];
    esp_err_t ret = i2c_master_transmit_receive(dev_handle_, &reg_addr, 1, buffer, 2, timeout_ms);
    if (ret == ESP_OK)
    {
        data = ((uint16_t)buffer[0] << 8) | buffer[1];  // Big Endian
        return true;
    }
    return false;
}

bool I2cDevice::WriteReg32(uint8_t reg_addr, uint32_t data, int timeout_ms)
{
    if (dev_handle_ == nullptr)
        return false;
    // Big Endian
    uint8_t buffer[5] = {reg_addr, (uint8_t)(data >> 24), (uint8_t)(data >> 16),
                         (uint8_t)(data >> 8), (uint8_t)(data & 0xFF)};
    return i2c_master_transmit(dev_handle_, buffer, 5, timeout_ms) == ESP_OK;
}

bool I2cDevice::ReadReg32(uint8_t reg_addr, uint32_t& data, int timeout_ms)
{
    if (dev_handle_ == nullptr)
        return false;
    uint8_t buffer[4];
    esp_err_t ret = i2c_master_transmit_receive(dev_handle_, &reg_addr, 1, buffer, 4, timeout_ms);
    if (ret == ESP_OK)
    {
        data = ((uint32_t)buffer[0] << 24) | ((uint32_t)buffer[1] << 16) |
               ((uint32_t)buffer[2] << 8) | buffer[3];  // Big Endian
        return true;
    }
    return false;
}

bool I2cDevice::WriteRegBit(uint8_t reg_addr, uint8_t bit, bool value, int timeout_ms)
{
    if (bit > 7)
        return false;
    uint8_t mask = 1 << bit;
    return WriteRegBits(reg_addr, mask, value ? mask : 0, timeout_ms);
}

bool I2cDevice::ReadRegBit(uint8_t reg_addr, uint8_t bit, bool& value, int timeout_ms)
{
    if (bit > 7)
        return false;
    uint8_t reg_value;
    if (ReadReg8(reg_addr, reg_value, timeout_ms))
    {
        value = (reg_value >> bit) & 0x01;
        return true;
    }
    return false;
}

bool I2cDevice::WriteRegBits(uint8_t reg_addr, uint8_t mask, uint8_t value, int timeout_ms)
{
    if (dev_handle_ == nullptr)
        return false;

    uint8_t current_value;
    if (!ReadReg8(reg_addr, current_value, timeout_ms))
        return false;

    uint8_t new_value = (current_value & ~mask) | (value & mask);
    return WriteReg8(reg_addr, new_value, timeout_ms);
}

bool I2cDevice::ReadRegBits(uint8_t reg_addr, uint8_t mask, uint8_t& value, int timeout_ms)
{
    uint8_t reg_value;
    if (ReadReg8(reg_addr, reg_value, timeout_ms))
    {
        value = reg_value & mask;
        return true;
    }
    return false;
}
