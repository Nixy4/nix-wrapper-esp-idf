#include "m5stack_tab5_keyboard.hpp"

#include <cstring>

namespace wrapper
{

Tab5Keyboard::Tab5Keyboard(Logger& logger)
    : I2cDevice(logger), current_mode_(Tab5KeyboardMode::Normal), key_callback_(nullptr)
{
}

bool Tab5Keyboard::Init(const I2cBus& bus)
{
    I2cDeviceConfig config(DEFAULT_ADDR, DEFAULT_SPEED);
    if (!I2cDevice::Init(bus, config))
    {
        return false;
    }

    // Verify communication by reading INT_CFG register
    uint8_t val = 0;
    if (!ReadReg8(REG_INT_CFG, val, kI2cTimeoutMs))
    {
        logger_.Error("Failed to read INT_CFG; device not responding");
        Deinit();
        return false;
    }

    logger_.Info("Initialized (INT_CFG: 0x%02X)", val);
    return true;
}

bool Tab5Keyboard::GetVersion(uint8_t& version)
{
    if (!ReadReg8(REG_VERSION, version, kI2cTimeoutMs))
    {
        logger_.Warning("Failed to read version");
        return false;
    }
    return true;
}

bool Tab5Keyboard::SetMode(Tab5KeyboardMode mode)
{
    const uint8_t val = static_cast<uint8_t>(mode);
    if (!WriteReg8(REG_KEYBOARD_MODE, val, kI2cTimeoutMs))
    {
        logger_.Warning("Failed to set keyboard mode");
        return false;
    }

    current_mode_ = mode;

    // Clear queues as mode switch resets device state
    uint8_t dummy = 0;
    GetInterruptStatus(dummy);
    ClearEventQueue();
    ClearInterruptStatus();

    return true;
}

bool Tab5Keyboard::GetMode(Tab5KeyboardMode& mode)
{
    uint8_t val = 0;
    if (!ReadReg8(REG_KEYBOARD_MODE, val, kI2cTimeoutMs))
    {
        logger_.Warning("Failed to read keyboard mode");
        return false;
    }
    mode = static_cast<Tab5KeyboardMode>(val);
    return true;
}

bool Tab5Keyboard::SetRgb(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index > 1)
    {
        logger_.Warning("SetRgb: invalid index %u", index);
        return false;
    }

    // On-device format is B, G, R
    // 寄存器布局: RGB1=0x60(B)/0x61(G)/0x62(R), 0x63保留, RGB2=0x64(B)/0x65(G)/0x66(R)
    // 步长为 4，不能用 index*3
    const uint8_t buf[3] = {b, g, r};
    const uint8_t reg_addr = static_cast<uint8_t>(REG_RGB_COLOR_BASE + index * 4u);
    if (!WriteRegBytes(reg_addr, {buf, buf + 3}, kI2cTimeoutMs))
    {
        logger_.Warning("SetRgb: I2C write failed");
        return false;
    }
    return true;
}

bool Tab5Keyboard::SetBothRgb(uint8_t r, uint8_t g, uint8_t b)
{
    // 7-byte window: [RGB1_B, RGB1_G, RGB1_R, Reserved(0), RGB2_B, RGB2_G, RGB2_R]
    const uint8_t buf[7] = {b, g, r, 0x00, b, g, r};
    if (!WriteRegBytes(REG_RGB_COLOR_BASE, {buf, buf + 7}, kI2cTimeoutMs))
    {
        logger_.Warning("SetBothRgb: I2C write failed");
        return false;
    }
    return true;
}

bool Tab5Keyboard::GetRgb(uint8_t index, uint8_t& r, uint8_t& g, uint8_t& b)
{
    if (index > 1)
    {
        logger_.Warning("GetRgb: invalid index %u", index);
        return false;
    }

    std::vector<uint8_t> buf;
    const uint8_t reg_addr = static_cast<uint8_t>(REG_RGB_COLOR_BASE + index * 4u);
    if (!ReadRegBytes(reg_addr, buf, 3, kI2cTimeoutMs))
    {
        logger_.Warning("GetRgb: I2C read failed");
        return false;
    }

    // On-device format is B, G, R
    b = buf[0];
    g = buf[1];
    r = buf[2];
    return true;
}

bool Tab5Keyboard::SetRgbMode(Tab5RgbMode mode)
{
    if (!WriteReg8(REG_RGB_MODE, static_cast<uint8_t>(mode), kI2cTimeoutMs))
    {
        logger_.Warning("Failed to set RGB mode");
        return false;
    }
    return true;
}

bool Tab5Keyboard::GetRgbMode(Tab5RgbMode& mode)
{
    uint8_t val = 0;
    if (!ReadReg8(REG_RGB_MODE, val, kI2cTimeoutMs))
    {
        logger_.Warning("Failed to read RGB mode");
        return false;
    }
    mode = static_cast<Tab5RgbMode>(val);
    return true;
}

bool Tab5Keyboard::SetBrightness(uint8_t brightness)
{
    if (brightness > 100)
    {
        logger_.Warning("SetBrightness: value %u exceeds 100", brightness);
        return false;
    }
    if (!WriteReg8(REG_BRIGHTNESS, brightness, kI2cTimeoutMs))
    {
        logger_.Warning("Failed to set brightness");
        return false;
    }
    return true;
}

bool Tab5Keyboard::GetBrightness(uint8_t& brightness)
{
    if (!ReadReg8(REG_BRIGHTNESS, brightness, kI2cTimeoutMs))
    {
        logger_.Warning("Failed to read brightness");
        return false;
    }
    return true;
}

bool Tab5Keyboard::GetInterruptStatus(uint8_t& status)
{
    if (!ReadReg8(REG_INT_STA, status, kI2cTimeoutMs))
    {
        logger_.Warning("Failed to read interrupt status");
        return false;
    }
    return true;
}

bool Tab5Keyboard::ClearInterruptStatus()
{
    if (!WriteReg8(REG_INT_STA, 0x00, kI2cTimeoutMs))
    {
        logger_.Warning("Failed to clear interrupt status");
        return false;
    }
    return true;
}

bool Tab5Keyboard::SetInterruptConfig(uint8_t config)
{
    if (!WriteReg8(REG_INT_CFG, config, kI2cTimeoutMs))
    {
        logger_.Warning("Failed to set interrupt config");
        return false;
    }
    return true;
}

bool Tab5Keyboard::GetInterruptConfig(uint8_t& config)
{
    if (!ReadReg8(REG_INT_CFG, config, kI2cTimeoutMs))
    {
        logger_.Warning("Failed to read interrupt config");
        return false;
    }
    return true;
}

bool Tab5Keyboard::GetEventCount(uint8_t& count)
{
    if (!ReadReg8(REG_EVENT_NUM, count, kI2cTimeoutMs))
    {
        logger_.Warning("Failed to read event count");
        return false;
    }
    return true;
}

bool Tab5Keyboard::ClearEventQueue()
{
    if (!WriteReg8(REG_EVENT_NUM, 0x00, kI2cTimeoutMs))
    {
        logger_.Warning("Failed to clear event queue");
        return false;
    }
    return true;
}

void Tab5Keyboard::SetKeyCallback(Tab5KeyCallback callback) { key_callback_ = std::move(callback); }

bool Tab5Keyboard::ReadEvent(Tab5KeyEvent& event)
{
    memset(&event, 0, sizeof(event));
    event.type = current_mode_;

    switch (current_mode_)
    {
        case Tab5KeyboardMode::Normal:
        {
            uint8_t raw = 0;
            if (!ReadReg8(REG_KEY_EVENT, raw, kI2cTimeoutMs))
            {
                return false;
            }
            if (raw == 0xFF)
            {
                return false;  // Invalid / empty
            }
            event.pressed = (raw & 0x80u) != 0u;
            event.row = (raw >> 4u) & 0x07u;
            event.col = raw & 0x0Fu;
            event.raw_data = raw;
            return true;
        }

        case Tab5KeyboardMode::Hid:
        case Tab5KeyboardMode::Ble:
        {
            std::vector<uint8_t> buf;
            if (!ReadRegBytes(REG_HID_EVENT, buf, 2, kI2cTimeoutMs))
            {
                return false;
            }
            if (buf[0] == 0xFF && buf[1] == 0xFF)
            {
                return false;  // Empty slot
            }
            event.hid_modifier = buf[0];
            event.hid_key_code = buf[1];
            return true;
        }

        case Tab5KeyboardMode::String:
        {
            uint8_t len = 0;
            if (!ReadReg8(REG_CHAR_EVENT_LEN, len, kI2cTimeoutMs) || len == 0)
            {
                return false;
            }
            if (len > 15)
            {
                logger_.Warning("String event length out of range: %u", len);
                return false;
            }
            std::vector<uint8_t> buf;
            if (!ReadRegBytes(REG_CHAR_EVENT_BASE, buf, static_cast<size_t>(len) + 1u,
                              kI2cTimeoutMs))
            {
                return false;
            }
            event.str_modifier = buf[0];
            event.str_len = len;
            memcpy(event.str_data, buf.data() + 1, len);
            event.str_data[len] = '\0';
            return true;
        }

        default:
            return false;
    }
}

bool Tab5Keyboard::Poll()
{
    uint8_t status = 0;
    if (!GetInterruptStatus(status))
    {
        return false;
    }

    const bool has_normal = (status & INT_STA_NORMAL) != 0u;
    const bool has_hid = (status & INT_STA_HID) != 0u;
    const bool has_string = (status & INT_STA_STRING) != 0u;

    const bool has_event = (current_mode_ == Tab5KeyboardMode::Normal && has_normal) ||
                           (current_mode_ == Tab5KeyboardMode::Hid && has_hid) ||
                           (current_mode_ == Tab5KeyboardMode::Ble && has_hid) ||
                           (current_mode_ == Tab5KeyboardMode::String && has_string);

    if (!has_event)
    {
        return true;
    }

    uint8_t count = 0;
    if (!GetEventCount(count) || count == 0)
    {
        ClearInterruptStatus();
        return true;
    }

    // Safety cap to prevent runaway reads on corrupted count
    if (count > 32)
    {
        count = 32;
    }

    while (count > 0)
    {
        Tab5KeyEvent event{};
        if (ReadEvent(event) && key_callback_)
        {
            key_callback_(event);
        }
        --count;
    }

    ClearInterruptStatus();
    return true;
}

bool Tab5Keyboard::SetI2cAddress(uint8_t new_addr)
{
    if (new_addr < 0x08 || new_addr > 0x77)
    {
        logger_.Warning("SetI2cAddress: invalid address 0x%02X", new_addr);
        return false;
    }
    if (!WriteReg8(REG_I2C_ADDR, new_addr, kI2cTimeoutMs))
    {
        logger_.Warning("Failed to set I2C address");
        return false;
    }
    return true;
}

bool Tab5Keyboard::GetI2cAddress(uint8_t& addr)
{
    if (!ReadReg8(REG_I2C_ADDR, addr, kI2cTimeoutMs))
    {
        logger_.Warning("Failed to read I2C address");
        return false;
    }
    return true;
}

}  // namespace wrapper
