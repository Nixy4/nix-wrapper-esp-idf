#include "m5stack_unit_extio2.hpp"

namespace wrapper
{

bool UnitExtio2::Init(const I2cBus& bus)
{
    I2cDeviceConfig cfg = I2cDeviceConfig(I2C_ADDR_DEFAULT, I2C_SPEED_HZ);
    return I2cDevice::Init(bus, cfg) == true;
}

void UnitExtio2::SetMode(int pin, Mode mode)
{
    GetLogger().Info("SetMode Pin %d: %d", pin, static_cast<uint8_t>(mode));
    WriteReg8(REG_MODE_IO(pin), static_cast<uint8_t>(mode), 1000);
}

UnitExtio2::Mode UnitExtio2::GetMode(int pin)
{
    uint8_t mode = 0xFF;
    if (ReadReg8(REG_MODE_IO(pin), mode, 1000) == true)
    {
        return static_cast<Mode>(mode);
    }
    return static_cast<Mode>(0xFF);
}

void UnitExtio2::SetModeAll(Mode mode)
{
    GetLogger().Info("SetMode All: %d", static_cast<uint8_t>(mode));
    for (int pin = 0; pin < 8; pin++)
    {
        WriteReg8(REG_MODE_IO(pin), static_cast<uint8_t>(mode), 1000);
    }
}

bool UnitExtio2::SetDigitalOutput(int pin, bool state)
{
    GetLogger().Debug("SetDigitalOutput: %d", state ? 1 : 0);
    return WriteReg8(REG_OUTPUT_CTL_IO(pin), state ? 1 : 0, 1000) == true;
}

bool UnitExtio2::SetDigitalOutputs(uint8_t states)
{
    GetLogger().Debug("SetDigitalOutputs: 0x%02X", states);
    return WriteReg8(Reg::OUTPUTS_CTL, states, 1000) == true;
}

bool UnitExtio2::GetDigitalInput(int pin, bool& state)
{
    uint8_t input_state = 0xFF;
    bool ret = ReadReg8(REG_DIGITAL_INPUT_IO(pin), input_state, 1000);

    if (ret == true)
    {
        GetLogger().Info("GetDigitalInput Pin %d: 0x%02X", pin, input_state);
        state = (input_state != 0);
    }
    else
    {
        GetLogger().Error("GetDigitalInput Pin %d failed!", pin);
    }
    return ret;
}
}  // namespace wrapper