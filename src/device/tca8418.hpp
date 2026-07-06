#pragma once

#include "local/i2c.hpp"
#include "local/logger.hpp"

namespace wrapper
{

/**
 * @brief TCA8418 I2C keyboard matrix controller driver
 *
 * Supports up to 8 rows x 10 columns. T-LoraPager uses 4 rows x 10 cols.
 * Default I2C address: 0x34.
 *
 * Key event FIFO holds up to 10 events. Each event byte:
 *   bit[7] = 1 pressed / 0 released
 *   bit[6:0] = key code (row * num_cols + col + 1)
 */
class Tca8418
{
   public:
    static constexpr uint8_t DEFAULT_ADDR = 0x34;
    static constexpr uint32_t DEFAULT_SPEED = 400000;

    // Register addresses
    static constexpr uint8_t REG_CFG = 0x01;          ///< Configuration
    static constexpr uint8_t REG_INT_STAT = 0x02;     ///< Interrupt status
    static constexpr uint8_t REG_KEY_LCK_EC = 0x03;   ///< Key lock / event count
    static constexpr uint8_t REG_KEY_EVENT_A = 0x04;  ///< Key event FIFO (first slot)
    static constexpr uint8_t REG_KP_GPIO1 = 0x1D;     ///< Row config port 1
    static constexpr uint8_t REG_KP_GPIO2 = 0x1E;     ///< Row config port 2
    static constexpr uint8_t REG_KP_GPIO3 = 0x1F;     ///< Row config port 3
    static constexpr uint8_t REG_GPI_EM1 = 0x09;      ///< GPI event mode reg 1
    static constexpr uint8_t REG_GPI_EM2 = 0x0A;      ///< GPI event mode reg 2
    static constexpr uint8_t REG_GPI_EM3 = 0x0B;      ///< GPI event mode reg 3
    static constexpr uint8_t REG_GPIO_DIR1 = 0x23;    ///< GPIO direction 1
    static constexpr uint8_t REG_GPIO_DIR2 = 0x24;    ///< GPIO direction 2
    static constexpr uint8_t REG_GPIO_DIR3 = 0x25;    ///< GPIO direction 3

    // CFG register bits
    static constexpr uint8_t CFG_AI = (1 << 7);            ///< Auto-increment
    static constexpr uint8_t CFG_GPI_E_CFG = (1 << 6);     ///< GPI event config
    static constexpr uint8_t CFG_OVR_FLOW_M = (1 << 5);    ///< Overflow mode
    static constexpr uint8_t CFG_INT_CFG = (1 << 4);       ///< Interrupt config
    static constexpr uint8_t CFG_OVR_FLOW_IEN = (1 << 3);  ///< Overflow interrupt enable
    static constexpr uint8_t CFG_K_LCK_IEN = (1 << 2);     ///< Key lock interrupt enable
    static constexpr uint8_t CFG_GPI_IEN = (1 << 1);       ///< GPI interrupt enable
    static constexpr uint8_t CFG_KE_IEN = (1 << 0);        ///< Key event interrupt enable

    // INT_STAT bits
    static constexpr uint8_t INT_K_LCK_STAT = (1 << 2);
    static constexpr uint8_t INT_GPI_STAT = (1 << 1);
    static constexpr uint8_t INT_K_INT = (1 << 0);

    explicit Tca8418(Logger& logger);
    ~Tca8418() = default;

    /**
     * @brief Initialize and configure the keyboard matrix.
     * @param rows Number of matrix rows (1-8)
     * @param cols Number of matrix columns (1-10)
     */
    bool Init(const I2cBus& bus, const I2cDeviceConfig& config, uint8_t rows, uint8_t cols);
    bool Deinit();

    /** @brief Returns true if at least one key event is pending in the FIFO */
    bool EventAvailable();

    /**
     * @brief Read one key event from FIFO.
     * @param key_code  Output: 1-based key code (row * cols + col + 1)
     * @param pressed   Output: true = pressed, false = released
     * @return true if an event was read, false if FIFO empty
     */
    bool GetKeyEvent(uint8_t& key_code, bool& pressed);

    /** @brief Clear the interrupt status register */
    bool ClearInterrupt();

    Logger& GetLogger() { return logger_; }

   private:
    Logger& logger_;
    I2cDevice device_;
    uint8_t rows_{0};
    uint8_t cols_{0};
};

}  // namespace wrapper
