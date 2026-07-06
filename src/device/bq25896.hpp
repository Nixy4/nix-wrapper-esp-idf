#pragma once

#include "local/i2c.hpp"
#include "local/logger.hpp"

namespace wrapper
{

/**
 * @brief BQ25896 I2C lithium battery charger driver (simplified)
 *
 * Covers the functionality needed for T-LoraPager initialisation:
 *   - Reset to defaults
 *   - Set charge target voltage
 *   - Set charge current
 *   - Enable / disable ADC measurement
 *
 * Default I2C address: 0x6B.
 */
class Bq25896
{
   public:
    static constexpr uint8_t DEFAULT_ADDR = 0x6B;
    static constexpr uint32_t DEFAULT_SPEED = 100000;

    // Selected register addresses
    static constexpr uint8_t REG00 = 0x00;  ///< Input source control
    static constexpr uint8_t REG02 = 0x02;  ///< ADC / charge config
    static constexpr uint8_t REG04 = 0x04;  ///< Charge current control
    static constexpr uint8_t REG06 = 0x06;  ///< Charge voltage control
    static constexpr uint8_t REG09 = 0x09;  ///< Operation control
    static constexpr uint8_t REG14 = 0x14;  ///< Device ID / part number

    explicit Bq25896(Logger& logger);
    ~Bq25896() = default;

    bool Init(const I2cBus& bus, const I2cDeviceConfig& config);
    bool Deinit();

    /** @brief Write register 0x09 bit 7 = 1 to trigger reset to defaults */
    bool ResetDefault();

    /**
     * @brief Set charge termination voltage.
     * @param mv  Target voltage in mV. Range 3840–4608 mV, step 16 mV.
     *            T-LoraPager recommended: 4288 mV.
     */
    bool SetChargeTargetVoltage(uint16_t mv);

    /**
     * @brief Set constant charge current limit.
     * @param ma  Current in mA. Range 0–5056 mA, step 64 mA.
     *            T-LoraPager recommended: 704 mA.
     */
    bool SetChargeCurrent(uint16_t ma);

    /** @brief Enable continuous ADC conversion (required for voltage/current readings) */
    bool EnableMeasure();

    /** @brief Disable ADC continuous conversion */
    bool DisableMeasure();

    /**
     * @brief Software power-off via BATFET disable (REG09 bit[5] = BATFET_DIS).
     *
     * Sets BATFET_DIS=1 to disconnect the battery FET and cut VSYS power.
     * Only effective when USB-C is NOT connected (BQ25896 hardware limitation).
     *
     * @param delayed  false = immediate cutoff;
     *                 true  = ~10 s delay (BATFET_DLY=1) to allow graceful shutdown.
     * @return true if the I²C write succeeded.
     */
    bool Shutdown(bool delayed = false);

    Logger& GetLogger() { return logger_; }

   private:
    Logger& logger_;
    I2cDevice device_;
};

}  // namespace wrapper
