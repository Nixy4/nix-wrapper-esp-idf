#pragma once

#include "local/logger.hpp"
#include "local/soc.hpp"
#include "local/i2c.hpp"
#include "local/spi.hpp"
#include "local/i2s.hpp"
#include "local/display.hpp"
#include "registry/touch.hpp"
#include "registry/lvgl.hpp"
#include "registry/audio.hpp"
#include "device/axp2101.hpp"
#include "device/aw9523.hpp"

namespace wrapper
{

class M5StackCoreS3
{
   private:
    bool initialized_ = false;

    M5StackCoreS3() = default;
    ~M5StackCoreS3() = default;
    M5StackCoreS3(const M5StackCoreS3&) = delete;
    M5StackCoreS3& operator=(const M5StackCoreS3&) = delete;
    M5StackCoreS3(M5StackCoreS3&&) = delete;
    M5StackCoreS3& operator=(M5StackCoreS3&&) = delete;

   public:
    static M5StackCoreS3& GetInstance()
    {
        static M5StackCoreS3 instance;
        return instance;
    }

    bool InitBus(bool i2c, bool spi, bool i2s);
    bool InitDevice(bool power, bool audio, bool display, bool touch);
    bool InitMiddleware(bool lvgl);
    bool Init();

    I2cBus& GetI2cBus1();
    SpiBus& GetSpiBus();
    I2sBus& GetI2sBus();

    Axp2101& GetPowerManagement();
    Aw9523& GetGpioExpander();
    LvglPort& GetLvglPort();
    AudioCodec& GetAudioCodec();
};

}  // namespace wrapper
