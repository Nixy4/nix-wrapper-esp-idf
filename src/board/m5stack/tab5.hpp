#pragma once

#include "local/logger.hpp"
#include "local/i2c.hpp"
#include "local/ledc.hpp"
#include "local/ldo.hpp"
#include "local/display-dsi.hpp"
#include "registry/touch.hpp"
#include "registry/audio.hpp"
#include "registry/lvgl.hpp"
#include "device/pi4ioe5v6408.hpp"
#include "device/ili9881c.hpp"
#include "device/gt911.hpp"
#include "device/m5stack_tab5_keyboard.hpp"

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

    bool InitCoreBusAndIoExpander();  ///< I2C0 总线 + IO Expander 实例化
    bool InitDisplay();               ///< LCD_EN/TOUCH_EN 引脚 + 背光 + DSI + 触摸 + LVGL
    bool InitAudio();                 ///< SPEAKER_EN 引脚 + I2S 总线 + AudioCodec
    bool InitKeyboard();              ///< 键盘 I2C1 总线 + 驱动

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

    M5Tab5Keyboard& GetKeyboard();
    void TestKeyboard();  ///< 键盘完整测试：初始化 + UI + 回调 + 轮询循环（阻塞）
};
}  // namespace wrapper