#pragma once

#include <array>
#include <u8g2.h>
#include "driver/gpio.h"
#include "local/i2c.hpp"
#include "local/spi.hpp"

namespace wrapper
{

struct U8g2I2cPortConfig
{
    uint32_t clk_hz = 400000;
    uint8_t device_addr_7bit = 0x3C;
    int timeout_ms = 1000;
    gpio_num_t reset_pin = GPIO_NUM_NC;
};

class U8g2I2cPort : public I2cDevice
{
    static U8g2I2cPort* active_port_;

    I2cBus& i2c_bus_;

    U8g2I2cPortConfig config_;
    u8g2_t u8g2_;
    bool initialized_;
    uint8_t current_i2c_addr_7bit_;
    std::array<uint8_t, 256> i2c_tx_buf_;
    uint16_t i2c_tx_len_;

    static bool IsValidPin(gpio_num_t pin);
    bool EnsureI2cDeviceForAddress(uint8_t addr_7bit);
    bool InitI2cPath();
    bool InitControlPins();

    uint8_t I2cByteCallback(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr);
    uint8_t GpioAndDelayCallback(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr);

   public:
    U8g2I2cPort(Logger& logger, I2cBus& i2c_bus);
    ~U8g2I2cPort();

    bool Init(const U8g2I2cPortConfig& config);
    bool Deinit();
    static uint8_t ByteCallback(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr);
    static uint8_t GpioAndDelayCallbackStatic(u8x8_t* u8x8,
                                              uint8_t msg,
                                              uint8_t arg_int,
                                              void* arg_ptr);
    u8g2_t* GetU8g2() { return &u8g2_; }
};

struct U8g2SpiPortConfig
{
    gpio_num_t cs_pin = GPIO_NUM_NC;
    gpio_num_t dc_pin = GPIO_NUM_NC;
    gpio_num_t reset_pin = GPIO_NUM_NC;
    uint8_t spi_mode = 0;
    uint32_t clock_hz = 10000000;
    int queue_size = 1;
};

class U8g2SpiPort : public SpiDevice
{
    static U8g2SpiPort* active_port_;

    SpiBus& spi_bus_;

    U8g2SpiPortConfig config_;
    u8g2_t u8g2_;
    bool initialized_;
    uint8_t dc_level_;

    static bool IsValidPin(gpio_num_t pin);
    bool InitSpiPath();
    bool InitControlPins();

    uint8_t SpiByteCallback(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr);
    uint8_t GpioAndDelayCallback(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr);

   public:
    U8g2SpiPort(Logger& logger, SpiBus& spi_bus);
    ~U8g2SpiPort();

    bool Init(const U8g2SpiPortConfig& config);
    bool Deinit();
    static uint8_t ByteCallback(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr);
    static uint8_t GpioAndDelayCallbackStatic(u8x8_t* u8x8,
                                              uint8_t msg,
                                              uint8_t arg_int,
                                              void* arg_ptr);
    u8g2_t* GetU8g2() { return &u8g2_; }
};

}  // namespace wrapper
