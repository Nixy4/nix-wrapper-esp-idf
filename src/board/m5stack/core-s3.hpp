#pragma once

#include "wrapper/logger.hpp"
#include "wrapper/soc.hpp"
#include "wrapper/i2c.hpp"
#include "wrapper/spi.hpp"
#include "wrapper/i2s.hpp"
#include "wrapper/display.hpp"
#include "wrapper/touch.hpp"
#include "wrapper/lvgl.hpp"
#include "wrapper/audio.hpp"
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
