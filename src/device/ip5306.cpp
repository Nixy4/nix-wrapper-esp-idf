#include "ip5306.hpp"

namespace wrapper
{

static constexpr int I2C_TIMEOUT_MS = 100;

bool Ip5306::Init(const I2cBus &bus)
{
    I2cDeviceConfig config(I2C_ADDR_DEFAULT, I2C_SPEED_HZ);
    return I2cDevice::Init(bus, config);
}

bool Ip5306::GetChargingStatus()
{
    bool charging = false;
    if (!ReadRegBit(REG_READ1, REG_READ1_BIT_CHARGE_STATUS, charging, I2C_TIMEOUT_MS))
    {
        logger_.Warning("Failed to read charging status");
        return false;
    }
    return charging;
}

bool Ip5306::SetChargerVoltage(ChargerVoltage voltage)
{
    constexpr uint8_t voltage_mask = 0b11;
    const uint8_t value = static_cast<uint8_t>(voltage);
    return WriteRegBits(REG_CHARGER_CTL0, voltage_mask, value, I2C_TIMEOUT_MS);
}

Ip5306::ChargerVoltage Ip5306::GetChargerVoltage()
{
    constexpr uint8_t voltage_mask = 0b11;
    uint8_t value = 0;

    if (!ReadRegBits(REG_CHARGER_CTL0, voltage_mask, value, I2C_TIMEOUT_MS))
    {
        logger_.Warning("Failed to read charger voltage, fallback to default");
        return ChargerVoltage::V_4_185_4_29_4_335_4_38;
    }

    return static_cast<ChargerVoltage>(value & voltage_mask);
}

}  // namespace wrapper
