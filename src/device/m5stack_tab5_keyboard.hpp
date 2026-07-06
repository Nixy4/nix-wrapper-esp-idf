#pragma once

#include <cstdint>
#include <functional>

#include "local/i2c.hpp"

namespace wrapper
{

// ============================================================
// 键盘工作模式
// ============================================================

/// @brief 键盘工作模式
/// Normal : 返回按键的行列坐标（raw matrix 原始坐标事件）
/// Hid    : 返回 USB HID modifier + keycode，可直接转发给主机
/// String : 返回键名字符串及 Ctrl/Alt 修饰符（Character 模式）
/// Ble    : BLE HID 转发模式（格式与 Hid 相同）
enum class M5Tab5KeyboardMode : uint8_t
{
    Normal = 0,
    Hid = 1,
    String = 2,
    Ble = 3,
};

/// @brief RGB LED 工作模式
/// Binding : 固件自动绑定 LED 颜色（用于指示当前键盘模式）
/// Custom  : 用户手动控制 LED 颜色
enum class M5Tab5RgbMode : uint8_t
{
    Binding = 0,
    Custom = 1,
};

// ============================================================
// 键盘事件结构体
// ============================================================

/// @brief 统一键盘事件，各字段按 type 分模式使用
struct M5Tab5KeyEvent
{
    M5Tab5KeyboardMode type;  ///< 事件来源模式

    // ---- Normal 模式字段 ----
    bool pressed;      ///< true=按下, false=释放
    uint8_t row;       ///< 矩阵行号 (0-4)
    uint8_t col;       ///< 矩阵列号 (0-13)
    uint8_t raw_data;  ///< 原始 1 字节事件 [7]=press [6:4]=row [3:0]=col

    // ---- HID / BLE 模式字段 ----
    uint8_t hid_modifier;  ///< HID modifier 字节 (Ctrl/Shift/Alt/GUI 组合)
    uint8_t hid_key_code;  ///< HID Usage Code (USB HID Keyboard Page 0x07)

    // ---- String (Character) 模式字段 ----
    uint8_t str_modifier;  ///< 修饰符: bit0=Ctrl, bit2=Alt
    uint8_t str_len;       ///< str_data 有效字节数
    char str_data[16];     ///< 键名字符串，NUL 终止，单字符或名称如 "Enter"
};

/// @brief 键盘事件回调类型
using M5Tab5KeyCallback = std::function<void(const M5Tab5KeyEvent&)>;

// ============================================================
// Tab5 键盘驱动
// M5Stack Tab5 专用 I2C 键盘 (SKU A164)
// 通信接口: I2C (SDA=GPIO0, SCL=GPIO1, INT=GPIO50)
// MCU: STM32F030C8T6，支持 14×5 矩阵，70 键
// ============================================================

class M5Tab5Keyboard : public I2cDevice
{
   public:
    // ---- 设备默认参数 ----
    static constexpr uint8_t DEFAULT_ADDR = 0x6D;      ///< 默认 I2C 地址
    static constexpr uint32_t DEFAULT_SPEED = 100000;  ///< 默认 I2C 频率 100 kHz

    // ---- 寄存器地址 ----
    static constexpr uint8_t REG_INT_CFG =
        0x00;  ///< 中断使能配置 (R/W) bit0=Normal bit1=HID bit2=String
    static constexpr uint8_t REG_INT_STA = 0x01;     ///< 中断状态/清除 (R/W) 写 0 清除
    static constexpr uint8_t REG_EVENT_NUM = 0x02;   ///< 事件队列长度 (R/W) 写 0 清空队列
    static constexpr uint8_t REG_BRIGHTNESS = 0x03;  ///< RGB 全局亮度 (R/W) 0–100，默认 20

    static constexpr uint8_t REG_KEYBOARD_MODE =
        0x10;                                      ///< 键盘工作模式 (R/W) 0=Normal 1=HID 2=String
    static constexpr uint8_t REG_RGB_MODE = 0x11;  ///< RGB 工作模式 (R/W) 0=Binding 1=Custom

    static constexpr uint8_t REG_KEY_EVENT = 0x20;  ///< Normal 模式事件读取 (R) 1 字节
    static constexpr uint8_t REG_HID_EVENT =
        0x30;  ///< HID 模式事件读取 (R) 2 字节 [modifier, keycode]

    static constexpr uint8_t REG_CHAR_EVENT_LEN = 0x40;  ///< String 模式事件长度 (R)
    static constexpr uint8_t REG_CHAR_EVENT_BASE =
        0x50;  ///< String 模式事件起始 (R) [modifier, chars...]

    static constexpr uint8_t REG_RGB_COLOR_BASE = 0x60;  ///< RGB 颜色寄存器基址 (R/W) BGR 格式

    static constexpr uint8_t REG_VERSION = 0xFE;   ///< 固件版本 (R)
    static constexpr uint8_t REG_I2C_ADDR = 0xFF;  ///< I2C 地址寄存器 (R/W) 0x08–0x77

    // ---- INT_STA 位掩码 ----
    static constexpr uint8_t INT_STA_NORMAL = (1u << 0);  ///< Normal 模式有事件待读
    static constexpr uint8_t INT_STA_HID = (1u << 1);     ///< HID 模式有事件待读
    static constexpr uint8_t INT_STA_STRING = (1u << 2);  ///< String 模式有事件待读

   public:
    explicit M5Tab5Keyboard(Logger& logger);
    ~M5Tab5Keyboard() = default;

    // ---- 初始化 ----

    /// @brief 初始化键盘（使用默认地址 0x6D / 100 kHz）
    /// @param bus 已初始化的 I2C 总线
    /// @return true 成功，false 通信失败
    bool Init(const I2cBus& bus);

    // ---- 设备信息 ----

    /// @brief 读取固件版本号（寄存器 0xFE）
    bool GetVersion(uint8_t& version);

    // ---- 工作模式 ----

    /// @brief 设置键盘工作模式；切换时会自动清空事件队列和中断状态
    bool SetMode(M5Tab5KeyboardMode mode);

    /// @brief 读取当前键盘工作模式
    bool GetMode(M5Tab5KeyboardMode& mode);

    // ---- RGB LED 控制（需先调用 SetRgbMode(Custom)）----

    /// @brief 设置单个 LED 颜色
    /// @param index LED 索引：0=左(RGB1), 1=右(RGB2)
    /// @param r/g/b 颜色分量 0–255
    bool SetRgb(uint8_t index, uint8_t r, uint8_t g, uint8_t b);

    /// @brief 同时设置两个 LED 为相同颜色
    bool SetBothRgb(uint8_t r, uint8_t g, uint8_t b);

    /// @brief 读取单个 LED 当前颜色
    bool GetRgb(uint8_t index, uint8_t& r, uint8_t& g, uint8_t& b);

    /// @brief 设置 RGB LED 工作模式（Binding=固件控制, Custom=用户控制）
    bool SetRgbMode(M5Tab5RgbMode mode);

    /// @brief 读取 RGB LED 当前工作模式
    bool GetRgbMode(M5Tab5RgbMode& mode);

    // ---- 背光亮度 ----

    /// @brief 设置 RGB 全局亮度，0（最暗）到 100（最亮）
    bool SetBrightness(uint8_t brightness);

    /// @brief 读取当前亮度值
    bool GetBrightness(uint8_t& brightness);

    // ---- 中断 / 状态寄存器 ----

    /// @brief 读取中断状态寄存器（INT_STA 0x01）
    /// @param status bit0=Normal bit1=HID bit2=String
    bool GetInterruptStatus(uint8_t& status);

    /// @brief 清除中断状态（写 0 至 INT_STA）
    bool ClearInterruptStatus();

    /// @brief 写入中断使能配置（INT_CFG 0x00）
    /// @param config bit0=Normal bit1=HID bit2=String
    bool SetInterruptConfig(uint8_t config);

    /// @brief 读取中断使能配置
    bool GetInterruptConfig(uint8_t& config);

    // ---- 事件队列 ----

    /// @brief 读取当前模式待处理事件数（0–32）
    bool GetEventCount(uint8_t& count);

    /// @brief 清空当前模式事件队列（写 0 至 EVENT_NUM）
    bool ClearEventQueue();

    // ---- 按键回调与轮询 ----

    /// @brief 注册按键事件回调；传入 nullptr 时清除回调
    void SetKeyCallback(M5Tab5KeyCallback callback);

    /// @brief 轮询一次键盘事件
    /// 读取 INT_STA → 按需读取全部事件 → 依次调用回调 → 清除 INT_STA
    /// 建议在定时任务或由 GPIO50（INT 引脚）触发时调用
    /// @return true 轮询正常执行（无 I2C 错误），false I2C 通信失败
    bool Poll();

    // ---- I2C 地址重配置（写入固件 Flash）----

    /// @brief 修改键盘 I2C 地址（写入 Flash，立即生效）
    /// @param new_addr 新地址，范围 0x08–0x77
    bool SetI2cAddress(uint8_t new_addr);

    /// @brief 读取当前 I2C 地址寄存器值
    bool GetI2cAddress(uint8_t& addr);

    // ---- 测试 ----

    /// @brief 切换到 String 模式，设置 LED 颜色，将按键事件打印到日志（阻塞轮询循环）
    /// @note 需先调用 Init()
    void Test();

   private:
    M5Tab5KeyboardMode current_mode_;  ///< 当前工作模式缓存
    M5Tab5KeyCallback key_callback_;   ///< 用户注册的事件回调

    static constexpr int kI2cTimeoutMs = 100;  ///< I2C 操作超时 ms

    /// @brief 从设备读取一条事件并填充 event（内部辅助函数）
    /// @return true 读取成功（含空队列情况），false I2C 错误
    bool ReadEvent(M5Tab5KeyEvent& event);
};

}  // namespace wrapper
