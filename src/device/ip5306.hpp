#pragma once
#include "local/i2c.hpp"

namespace wrapper
{

class Ip5306 : public I2cDevice
{
   public:
    static constexpr uint8_t I2C_ADDR_DEFAULT = 0x75;
    static constexpr uint32_t I2C_SPEED_HZ = 100000;

    static constexpr uint8_t REG_SYS_CTL0 = 0x00;
    static constexpr uint8_t REG_SYS_CTL1 = 0x01;
    static constexpr uint8_t REG_SYS_CTL2 = 0x02;

    static constexpr uint8_t REG_CHARGER_CTL0 = 0x20;
    static constexpr uint8_t REG_CHARGER_CTL1 = 0x21;
    static constexpr uint8_t REG_CHARGER_CTL2 = 0x22;
    static constexpr uint8_t REG_CHARGER_CTL3 = 0x23;

    static constexpr uint8_t REG_CHG_DIG_CTL0 = 0x24;

    static constexpr uint8_t REG_READ0 = 0x70;
    static constexpr uint8_t REG_READ0_BIT_CHARGE_EN = 3;  // 充电使能标志

    static constexpr uint8_t REG_READ1 = 0x71;
    static constexpr uint8_t REG_READ1_BIT_CHARGE_STATUS = 3;

    static constexpr uint8_t REG_READ2 = 0x72;
    static constexpr uint8_t REG_READ3 = 0x77;

    // Charger voltage cutoff settings (REG_CHARGER_CTL0 bits 1:0)
    enum class ChargerVoltage : uint8_t
    {
        V_4_14_4_26_4_305_4_35 = 0b00,   // 建议使用
        V_4_17_4_275_4_32_4_365 = 0b01,  // 建议使用
        V_4_185_4_29_4_335_4_38 = 0b10,  // 默认值
        V_4_2_4_305_4_35_4_395 = 0b11
    };

    Ip5306(Logger& logger) : I2cDevice(logger) {}

    ~Ip5306() = default;

    bool Init(const I2cBus& bus);
    bool GetChargingStatus();

    bool SetChargerVoltage(ChargerVoltage voltage);

    ChargerVoltage GetChargerVoltage();
};

}  // namespace wrapper