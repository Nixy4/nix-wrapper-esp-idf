#include "misc/u8g2-hal-esp-idf.hpp"

#include <cstring>

#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace wrapper
{

static uint8_t HandleDelayMessage(uint8_t msg, uint8_t arg_int)
{
    switch (msg)
    {
        case U8X8_MSG_DELAY_MILLI:
            vTaskDelay(pdMS_TO_TICKS(arg_int));
            return 1;

        case U8X8_MSG_DELAY_10MICRO:
            esp_rom_delay_us(10U * arg_int);
            return 1;

        case U8X8_MSG_DELAY_NANO:
        case U8X8_MSG_DELAY_100NANO:
            return 1;

        default:
            return 0;
    }
}

U8g2I2cPort* U8g2I2cPort::active_port_ = nullptr;

U8g2I2cPort::U8g2I2cPort(Logger& logger, I2cBus& i2c_bus)
    : I2cDevice(logger),
      i2c_bus_(i2c_bus),
      config_{},
      u8g2_{},
      initialized_(false),
      current_i2c_addr_7bit_(0),
      i2c_tx_buf_{},
      i2c_tx_len_(0)
{
    std::memset(&u8g2_, 0, sizeof(u8g2_));
}

U8g2I2cPort::~U8g2I2cPort() { Deinit(); }

bool U8g2I2cPort::IsValidPin(gpio_num_t pin)
{
    return (pin >= GPIO_NUM_0) && (pin < GPIO_NUM_MAX) && (pin != GPIO_NUM_NC);
}

bool U8g2I2cPort::Init(const U8g2I2cPortConfig& config)
{
    if (initialized_)
    {
        Deinit();
    }

    config_ = config;
    current_i2c_addr_7bit_ = config_.device_addr_7bit;
    i2c_tx_len_ = 0;

    active_port_ = this;
    initialized_ = true;
    return true;
}

bool U8g2I2cPort::Deinit()
{
    bool ok = I2cDevice::Deinit();

    if (active_port_ == this)
    {
        active_port_ = nullptr;
    }
    initialized_ = false;
    i2c_tx_len_ = 0;
    return ok;
}

bool U8g2I2cPort::EnsureI2cDeviceForAddress(uint8_t addr_7bit)
{
    if (current_i2c_addr_7bit_ == addr_7bit)
    {
        return true;
    }

    if (!I2cDevice::Deinit())
    {
        logger_.Error("Failed to switch I2C address: device deinit failed");
        return false;
    }

    I2cDeviceConfig dev_cfg(addr_7bit, config_.clk_hz);
    if (!I2cDevice::Init(i2c_bus_, dev_cfg))
    {
        logger_.Error("Failed to switch I2C address to 0x%02X", addr_7bit);
        return false;
    }

    current_i2c_addr_7bit_ = addr_7bit;
    return true;
}

bool U8g2I2cPort::InitI2cPath()
{
    if (i2c_bus_.GetHandle() == nullptr)
    {
        logger_.Error("I2C bus is not initialized");
        return false;
    }

    current_i2c_addr_7bit_ = config_.device_addr_7bit;
    I2cDeviceConfig dev_cfg(current_i2c_addr_7bit_, config_.clk_hz);
    return I2cDevice::Init(i2c_bus_, dev_cfg);
}

bool U8g2I2cPort::InitControlPins()
{
    if (!IsValidPin(config_.reset_pin))
    {
        return true;
    }

    gpio_config_t io_cfg = {
        .pin_bit_mask = (1ULL << config_.reset_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    return gpio_config(&io_cfg) == ESP_OK;
}

uint8_t U8g2I2cPort::I2cByteCallback(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr)
{
    switch (msg)
    {
        case U8X8_MSG_BYTE_INIT:
            return InitI2cPath() ? 1 : 0;

        case U8X8_MSG_BYTE_SET_DC:
            return 1;

        case U8X8_MSG_BYTE_START_TRANSFER:
        {
            uint8_t i2c_addr_8bit = u8x8_GetI2CAddress(u8x8);
            uint8_t i2c_addr_7bit = static_cast<uint8_t>(i2c_addr_8bit >> 1);
            if (!EnsureI2cDeviceForAddress(i2c_addr_7bit))
            {
                return 0;
            }
            i2c_tx_len_ = 0;
            return 1;
        }

        case U8X8_MSG_BYTE_SEND:
        {
            if (arg_ptr == nullptr || arg_int == 0)
            {
                return 1;
            }

            const uint8_t* src = static_cast<const uint8_t*>(arg_ptr);
            uint16_t remaining = arg_int;

            while (remaining > 0)
            {
                const uint16_t room = static_cast<uint16_t>(i2c_tx_buf_.size() - i2c_tx_len_);
                const uint16_t chunk = remaining < room ? remaining : room;

                std::memcpy(&i2c_tx_buf_[i2c_tx_len_], src, chunk);
                i2c_tx_len_ += chunk;
                src += chunk;
                remaining -= chunk;

                if (i2c_tx_len_ == i2c_tx_buf_.size())
                {
                    if (!WriteBytes(i2c_tx_buf_.data(), i2c_tx_len_, config_.timeout_ms))
                    {
                        i2c_tx_len_ = 0;
                        return 0;
                    }
                    i2c_tx_len_ = 0;
                }
            }
            return 1;
        }

        case U8X8_MSG_BYTE_END_TRANSFER:
        {
            if (i2c_tx_len_ == 0)
            {
                return 1;
            }

            const bool ok = WriteBytes(i2c_tx_buf_.data(), i2c_tx_len_, config_.timeout_ms);
            i2c_tx_len_ = 0;
            return ok ? 1 : 0;
        }

        default:
            return 1;
    }
}

uint8_t U8g2I2cPort::GpioAndDelayCallback(u8x8_t* u8x8,
                                          uint8_t msg,
                                          uint8_t arg_int,
                                          void* arg_ptr)
{
    (void)u8x8;
    (void)arg_ptr;

    switch (msg)
    {
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            return InitControlPins() ? 1 : 0;

        case U8X8_MSG_GPIO_RESET:
            if (IsValidPin(config_.reset_pin))
            {
                gpio_set_level(config_.reset_pin, arg_int);
            }
            return 1;

        default:
        {
            const uint8_t handled = HandleDelayMessage(msg, arg_int);
            return handled ? handled : 1;
        }
    }
}

uint8_t U8g2I2cPort::ByteCallback(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr)
{
    U8g2I2cPort* self = active_port_;
    if (self == nullptr || !self->initialized_)
    {
        return 0;
    }

    return self->I2cByteCallback(u8x8, msg, arg_int, arg_ptr);
}

uint8_t U8g2I2cPort::GpioAndDelayCallbackStatic(u8x8_t* u8x8,
                                                uint8_t msg,
                                                uint8_t arg_int,
                                                void* arg_ptr)
{
    U8g2I2cPort* self = active_port_;
    if (self == nullptr || !self->initialized_)
    {
        return 0;
    }

    return self->GpioAndDelayCallback(u8x8, msg, arg_int, arg_ptr);
}

U8g2SpiPort* U8g2SpiPort::active_port_ = nullptr;

U8g2SpiPort::U8g2SpiPort(Logger& logger, SpiBus& spi_bus)
    : SpiDevice(logger),
      spi_bus_(spi_bus),
      config_{},
      u8g2_{},
      initialized_(false),
      dc_level_(0)
{
    std::memset(&u8g2_, 0, sizeof(u8g2_));
}

U8g2SpiPort::~U8g2SpiPort() { Deinit(); }

bool U8g2SpiPort::IsValidPin(gpio_num_t pin)
{
    return (pin >= GPIO_NUM_0) && (pin < GPIO_NUM_MAX) && (pin != GPIO_NUM_NC);
}

bool U8g2SpiPort::Init(const U8g2SpiPortConfig& config)
{
    if (initialized_)
    {
        Deinit();
    }

    config_ = config;
    dc_level_ = 0;
    active_port_ = this;
    initialized_ = true;
    return true;
}

bool U8g2SpiPort::Deinit()
{
    bool ok = SpiDevice::Deinit();

    if (active_port_ == this)
    {
        active_port_ = nullptr;
    }
    initialized_ = false;
    return ok;
}

bool U8g2SpiPort::InitSpiPath()
{
    SpiDeviceConfig dev_cfg(config_.cs_pin, static_cast<int>(config_.clock_hz), config_.spi_mode);
    dev_cfg.queue_size = config_.queue_size;
    return SpiDevice::Init(spi_bus_, dev_cfg);
}

bool U8g2SpiPort::InitControlPins()
{
    uint64_t pin_mask = 0;
    if (IsValidPin(config_.dc_pin))
    {
        pin_mask |= (1ULL << config_.dc_pin);
    }
    if (IsValidPin(config_.reset_pin))
    {
        pin_mask |= (1ULL << config_.reset_pin);
    }

    if (pin_mask == 0)
    {
        return true;
    }

    gpio_config_t io_cfg = {
        .pin_bit_mask = pin_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    return gpio_config(&io_cfg) == ESP_OK;
}

uint8_t U8g2SpiPort::SpiByteCallback(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr)
{
    (void)u8x8;

    switch (msg)
    {
        case U8X8_MSG_BYTE_INIT:
            return InitSpiPath() ? 1 : 0;

        case U8X8_MSG_BYTE_SET_DC:
            dc_level_ = arg_int ? 1 : 0;
            if (IsValidPin(config_.dc_pin))
            {
                gpio_set_level(config_.dc_pin, dc_level_);
            }
            return 1;

        case U8X8_MSG_BYTE_START_TRANSFER:
            return 1;

        case U8X8_MSG_BYTE_SEND:
            if (arg_ptr == nullptr || arg_int == 0)
            {
                return 1;
            }
            return Write(static_cast<const uint8_t*>(arg_ptr), arg_int) ? 1 : 0;

        case U8X8_MSG_BYTE_END_TRANSFER:
            return 1;

        default:
            return 1;
    }
}

uint8_t U8g2SpiPort::GpioAndDelayCallback(u8x8_t* u8x8,
                                          uint8_t msg,
                                          uint8_t arg_int,
                                          void* arg_ptr)
{
    (void)u8x8;
    (void)arg_ptr;

    switch (msg)
    {
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            return InitControlPins() ? 1 : 0;

        case U8X8_MSG_GPIO_DC:
            if (IsValidPin(config_.dc_pin))
            {
                dc_level_ = arg_int ? 1 : 0;
                gpio_set_level(config_.dc_pin, dc_level_);
            }
            return 1;

        case U8X8_MSG_GPIO_RESET:
            if (IsValidPin(config_.reset_pin))
            {
                gpio_set_level(config_.reset_pin, arg_int);
            }
            return 1;

        default:
        {
            const uint8_t handled = HandleDelayMessage(msg, arg_int);
            return handled ? handled : 1;
        }
    }
}

uint8_t U8g2SpiPort::ByteCallback(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr)
{
    (void)u8x8;
    U8g2SpiPort* self = active_port_;
    if (self == nullptr || !self->initialized_)
    {
        return 0;
    }

    return self->SpiByteCallback(u8x8, msg, arg_int, arg_ptr);
}

uint8_t U8g2SpiPort::GpioAndDelayCallbackStatic(u8x8_t* u8x8,
                                                uint8_t msg,
                                                uint8_t arg_int,
                                                void* arg_ptr)
{
    U8g2SpiPort* self = active_port_;
    if (self == nullptr || !self->initialized_)
    {
        return 0;
    }

    return self->GpioAndDelayCallback(u8x8, msg, arg_int, arg_ptr);
}

}  // namespace wrapper
