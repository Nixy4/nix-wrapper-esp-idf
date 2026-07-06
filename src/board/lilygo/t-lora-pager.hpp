#pragma once

#include "sdkconfig.h"
#include "wrapper/logger.hpp"
#include "wrapper/i2c.hpp"
#include "wrapper/spi.hpp"
#include "wrapper/i2s.hpp"
#include "wrapper/ledc.hpp"
#include "wrapper/display.hpp"
#include "registry/audio.hpp"
#include "registry/lvgl.hpp"
#include "wrapper/encoder.hpp"
#include "wrapper/sd_spi.hpp"
#include "device/xl9555.hpp"
#include "device/st7796.hpp"
#include "device/tca8418.hpp"
#include "device/bq25896.hpp"
#include "device/lilygo_t_lora_pager_keyboard.hpp"

namespace wrapper
{

/**
 * @brief LilyGo T-LoraPager 板级支持类 (ESP32-S3)
 *
 * 单例，拥有并初始化所有板载外设：
 *   - I²C 总线 (SDA=3, SCL=2) — 由 XL9555、BQ25896、TCA8418、ES8311 共用
 *   - 共享 SPI 总线 (MOSI=34, MISO=33, SCK=35) — ST7796、LoRa、NFC、SD
 *   - I²S 总线 (MCLK=10, SCK=11, WS=18, Dout=45, Din=17) — ES8311 编解码器
 *   - XL9555 16 位 IO 扩展器 (addr=0x20) — 外设电源控制
 *   - ST7796 SPI LCD (480×222, CS=38, DC=37, BL=42)
 *   - 基于 ST7796 的 LVGL 移植层
 *   - ES8311 音频编解码器 via AudioCodec
 *   - TCA8418 键盘矩阵控制器 (addr=0x34, INT=GPIO6)
 *   - BQ25896 电池充电器 (addr=0x6B)
 *   - SdSpi SD 卡（SPI 模式，CS=GPIO21），VFS FAT 挂载到 /sdcard
 *
 * 用法：
 * @code
 *   auto& board = wrapper::LilyGoLoraPager::GetInstance();
 *   board.InitCoreBusAndIoExpander();
 *   board.InitDisplay();
 *   board.GetLvglPort().SetRotation(LV_DISPLAY_ROTATION_0);
 *   board.InitAudio();
 *   board.InitKeyboard();
 *   board.InitSdCard();
 * @endcode
 */
class LilyGoLoraPager
{
   private:
    LilyGoLoraPager() = default;
    ~LilyGoLoraPager() = default;

    LilyGoLoraPager(const LilyGoLoraPager&) = delete;
    LilyGoLoraPager& operator=(const LilyGoLoraPager&) = delete;
    LilyGoLoraPager(LilyGoLoraPager&&) = delete;
    LilyGoLoraPager& operator=(LilyGoLoraPager&&) = delete;

   public:
    static LilyGoLoraPager& GetInstance()
    {
        static LilyGoLoraPager instance;
        return instance;
    }

    std::atomic<bool> shutdown_requested{false};

    // -----------------------------------------------------------------------
    // 初始化 — 按顺序调用
    // -----------------------------------------------------------------------

    bool InitBootButton();  // GPIO0 低电平触发软件关机

    /** @brief 初始化 I²C 总线 + XL9555 IO 扩展器 + BQ25896 充电器 */
    bool InitCoreBusAndIoExpander();

    /**
     * @brief 初始化共享 SPI 总线 + ST7796 显示屏 + LEDC 背光 + LVGL。
     *
     * 同时将 LoRa / NFC / SD 片选 GPIO 拉高，
     * 避免干扰 SPI 总线初始化。
     */
    bool InitDisplay();

    /** @brief 初始化 I²S 总线 + ES8311 音频编解码器（扬声器 + 麦克风）*/
    bool InitAudio();

    /** @brief 初始化 TCA8418 键盘矩阵控制器（4 行 × 10 列）*/
    bool InitKeyboard();

    /**
     * @brief 初始化旋转编码器并将其注册为 LVGL 输入设备。
     *
     * 引脚分配：A = GPIO40，B = GPIO41，按鈕 = GPIO7（与 SEL_BTN 共用）。
     * 同时创建 LVGL 焦点组并设为默认组。
     *
     * @note 必须在 InitDisplay() 之后调用。
     */
    bool InitEncoder();

    /**
     * @brief 初始化 SD 卡（SPI 模式）并将 FAT 文件系统挂载到 VFS /sdcard。
     *
     * 通过 XL9555 使能 SD 电源轨，然后在共享 SPI 总线（CS=GPIO21）上
     * 完成卡识别与 FAT 挂载。挂载成功后可通过 POSIX/C 文件 API
     * 访问 "/sdcard/..."。
     *
     * @note 必须在 InitDisplay()（SPI 总线）和 InitCoreBusAndIoExpander()
     *       （XL9555）之后调用。
     */
    bool InitSdCard();

    // -----------------------------------------------------------------------
    // 获取器
    // -----------------------------------------------------------------------

    I2cBus& GetI2cBus();
    SpiBus& GetSpiBus();
    I2sBus& GetI2sBus();
    Xl9555& GetIoExpander();
    St7796& GetDisplay();
    LedcTimer& GetLedcTimer();
    LedcChannel& GetLedcChannel();
    AudioCodec& GetAudioCodec();
    LvglPort& GetLvglPort();
    Tca8418& GetKeyboard();
    Bq25896& GetPmu();
    Encoder& GetEncoder();
    LilyGoLoRaPagerKeyboard& GetKeyboardDriver();
    SdSpi& GetSdCard();

    // -----------------------------------------------------------------------
    // 便捷辅助方法
    // -----------------------------------------------------------------------

    /** @brief 设置显示屏背光亮度 0–100% */
    void SetDisplayBrightness(int percent);

    /** @brief 打开 / 关闭背光 */
    void SetDisplayBacklight(bool on);

    /**
     * @brief 通过 XL9555 控制外设电源轨。
     *
     * @param xl9555_pin  IO 扩展器引脚编号 (0-15)。原理图确认后，
     *                    使用 t-lora-pager.cpp 中定义的 kXl9555Pin* 常量。
     * @param enable      true = 高电平（使能），false = 低电平（禁用）
     */
    bool SetPeripheralPower(uint32_t xl9555_pin, bool enable);

    void BootButtonHandler();
    void KeyboardHandler();
};

}  // namespace wrapper
