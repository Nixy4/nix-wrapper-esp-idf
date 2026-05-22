#pragma once

#include "wrapper/logger.hpp"
#include "wrapper/i2c.hpp"
#include "wrapper/display.hpp"

namespace wrapper
{

class UnitExtio2 : public I2cDevice
{
    enum Reg : uint8_t
    {
        MODE_BASE = 0x00,
        MODE_IO0 = 0x00,
        MODE_IO1,
        MODE_IO2,
        MODE_IO3,
        MODE_IO4,
        MODE_IO5,
        MODE_IO6,
        MODE_IO7,

        OUTPUT_CTL_BASE = 0x10,
        OUTPUT_CTL_IO0 = 0x10,
        OUTPUT_CTL_IO1,
        OUTPUT_CTL_IO2,
        OUTPUT_CTL_IO3,
        OUTPUT_CTL_IO4,
        OUTPUT_CTL_IO5,
        OUTPUT_CTL_IO6,
        OUTPUT_CTL_IO7,
        OUTPUTS_CTL = 0x18,

        DIGITAL_INPUT_BASE = 0x20,
        DIGITAL_INPUT_IO0 = 0x20,
        DIGITAL_INPUT_IO1,
        DIGITAL_INPUT_IO2,
        DIGITAL_INPUT_IO3,
        DIGITAL_INPUT_IO4,
        DIGITAL_INPUT_IO5,
        DIGITAL_INPUT_IO6,
        DIGITAL_INPUT_IO7,
        DIGITAL_INPUTS = 0x28,

        ANALOG_INPUT_8B = 0x30,
        ANALOG_INPUT_12B = 0x40,

        SERVO_ANGLE_8B = 0x50,
        SERVO_PULSE_16B = 0x60,

        RGB_24B = 0x70,

        FW_VERSION = 0xFE,
        ADDRESS = 0xFF
    };

    constexpr static inline uint8_t REG_MODE_IO(uint8_t pin)
    {
        return static_cast<uint8_t>(Reg::MODE_BASE) + pin;
    }

    constexpr static inline uint8_t REG_OUTPUT_CTL_IO(uint8_t pin)
    {
        return static_cast<uint8_t>(Reg::OUTPUT_CTL_BASE) + pin;
    }

    constexpr static inline uint8_t REG_DIGITAL_INPUT_IO(uint8_t pin)
    {
        return static_cast<uint8_t>(Reg::DIGITAL_INPUT_BASE) + pin;
    }

    constexpr static inline uint8_t REG_ANALOG_INPUT_8B(uint8_t pin)
    {
        return static_cast<uint8_t>(Reg::ANALOG_INPUT_8B) + pin;
    }

    constexpr static inline uint8_t REG_ANALOG_INPUT_12B(uint8_t pin)
    {
        return static_cast<uint8_t>(Reg::ANALOG_INPUT_12B) + pin;
    }

   public:
    static constexpr uint8_t I2C_ADDR_DEFAULT = 0x45;
    static constexpr uint32_t I2C_SPEED_HZ = 400000;

    enum Mode : uint8_t
    {
        DIGITAL_INPUT = 0,
        DIGITAL_OUTPUT,
        ADC_INPUT,
        SERVO_CTL,
        RGB_LED
    };

    enum AnalogReadMode : uint8_t
    {
        BITS8 = 0,
        BITS12 = 1
    };

    UnitExtio2(Logger& logger) : I2cDevice(logger) {}

    ~UnitExtio2() = default;

    bool Init(const I2cBus& bus);

    void SetMode(int pin, Mode mode);

    Mode GetMode(int pin);

    void SetModeAll(Mode mode);

    bool SetDigitalOutput(int pin, bool state);

    bool SetDigitalOutputs(uint8_t states);
    bool GetDigitalInput(int pin, bool& state);
};

}  // namespace wrapper