#pragma once

#include "wrapper/logger.hpp"
#include "wrapper/i2c.hpp"
#include "wrapper/ledc.hpp"
#include "wrapper/ldo.hpp"
#include "wrapper/display-dsi.hpp"
#include "wrapper/touch.hpp"
#include "wrapper/audio.hpp"
#include "wrapper/lvgl.hpp"
#include "device/pi4ioe5v6408.hpp"
#include "device/ili9881c.hpp"
#include "device/gt911.hpp"

namespace wrapper
{
class M5StackTab5
{
   private:
    M5StackTab5() = default;
    ~M5StackTab5() = default;

    M5StackTab5(const M5StackTab5&) = delete;
    M5StackTab5& operator=(const M5StackTab5&) = delete;
    M5StackTab5(M5StackTab5&&) = delete;
    M5StackTab5& operator=(M5StackTab5&&) = delete;

   public:
    static M5StackTab5& GetInstance()
    {
        static M5StackTab5 instance;
        return instance;
    }

    bool Init(int level);

    I2cBus& GetI2cBus();
    Pi4ioe5v6408& GetIoExpander0();
    Pi4ioe5v6408& GetIoExpander1();
    LedcTimer& GetLedcTimer();
    LedcChannel& GetLedcChannel();
    LdoRegulator& GetDsiPhyLdo();
    DsiBus& GetDsiBus();
    Ili9881c& GetDsiDisplay();
    Gt911& GetGt911Touch();
    I2sBus& GetI2sBus();
    AudioCodec& GetAudioCodec();
    LvglPort& GetLvglPort();

    void SetDisplayBrightness(int percent);
    void SetDisplayBacklight(bool on);
    void SetDisplayPower(bool on);
};
}  // namespace wrapper