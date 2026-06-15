#include "bq25896.hpp"

namespace wrapper
{

Bq25896::Bq25896(Logger& logger) : logger_(logger), device_(logger) {}

bool Bq25896::Init(const I2cBus& bus, const I2cDeviceConfig& config)
{
    if (!device_.Init(bus, config))
    {
        logger_.Error("Failed to initialize I2C device");
        return false;
    }

    // Probe: read device ID register (REG14 bits[2:0] should be 0x00 for BQ25896)
    uint8_t id = 0;
    if (!device_.ReadReg8(REG14, id, -1))
    {
        logger_.Error("Failed to read device ID");
        return false;
    }

    logger_.Info("BQ25896 initialized (addr: 0x%02x, ID: 0x%02x)", config.device_address, id);
    return true;
}

bool Bq25896::Deinit() { return device_.Deinit(); }

bool Bq25896::ResetDefault()
{
    // REG09 bit[7] = 1 triggers register reset
    uint8_t val = 0;
    if (!device_.ReadReg8(REG09, val, -1))
        return false;
    val |= 0x80;
    if (!device_.WriteReg8(REG09, val, -1))
    {
        logger_.Error("Failed to write REG09 for reset");
        return false;
    }
    logger_.Info("BQ25896 reset to defaults");
    return true;
}

bool Bq25896::SetChargeTargetVoltage(uint16_t mv)
{
    // REG06[7:2] = VREG[5:0], offset 3840 mV, step 16 mV
    // VREG = (mv - 3840) / 16  (clamped to [0, 63])
    if (mv < 3840)
        mv = 3840;
    if (mv > 4608)
        mv = 4608;

    uint8_t vreg = static_cast<uint8_t>((mv - 3840) / 16);
    uint8_t val = 0;
    if (!device_.ReadReg8(REG06, val, -1))
        return false;

    val = static_cast<uint8_t>((val & 0x03) | (vreg << 2));
    if (!device_.WriteReg8(REG06, val, -1))
    {
        logger_.Error("Failed to set charge voltage");
        return false;
    }
    logger_.Info("Charge target voltage set to %u mV (VREG=0x%02x)", mv, vreg);
    return true;
}

bool Bq25896::SetChargeCurrent(uint16_t ma)
{
    // REG04[6:0] = ICHG[6:0], offset 0 mA, step 64 mA
    // ICHG = ma / 64  (clamped to [0, 79])
    if (ma > 5056)
        ma = 5056;

    uint8_t ichg = static_cast<uint8_t>(ma / 64);
    uint8_t val = 0;
    if (!device_.ReadReg8(REG04, val, -1))
        return false;

    val = static_cast<uint8_t>((val & 0x80) | (ichg & 0x7F));
    if (!device_.WriteReg8(REG04, val, -1))
    {
        logger_.Error("Failed to set charge current");
        return false;
    }
    logger_.Info("Charge current set to %u mA (ICHG=0x%02x)", ma, ichg);
    return true;
}

bool Bq25896::EnableMeasure()
{
    // REG02[7] = CONV_RATE=1 (continuous ADC)
    uint8_t val = 0;
    if (!device_.ReadReg8(REG02, val, -1))
        return false;
    val |= 0x80;
    if (!device_.WriteReg8(REG02, val, -1))
    {
        logger_.Error("Failed to enable ADC measurement");
        return false;
    }
    return true;
}

bool Bq25896::DisableMeasure()
{
    // REG02[7] = CONV_RATE=0
    uint8_t val = 0;
    if (!device_.ReadReg8(REG02, val, -1))
        return false;
    val &= ~0x80;
    if (!device_.WriteReg8(REG02, val, -1))
    {
        logger_.Error("Failed to disable ADC measurement");
        return false;
    }
    return true;
}

bool Bq25896::Shutdown(bool delayed)
{
    // REG09[5] = BATFET_DIS : 1 = disconnect BATFET → VSYS loses power
    // REG09[3] = BATFET_DLY : 0 = immediate, 1 = ~10 s delay
    // NOTE: Has no effect when USB-C VBUS is present.
    uint8_t val = 0;
    if (!device_.ReadReg8(REG09, val, -1))
        return false;

    if (delayed)
        val |= (1u << 3);  // BATFET_DLY
    else
        val &= ~(1u << 3);  // immediate

    val |= (1u << 5);  // BATFET_DIS

    if (!device_.WriteReg8(REG09, val, -1))
    {
        logger_.Error("Failed to write BATFET_DIS");
        return false;
    }
    logger_.Info("Shutdown requested (delayed=%d)", delayed ? 1 : 0);
    return true;
}

}  // namespace wrapper
