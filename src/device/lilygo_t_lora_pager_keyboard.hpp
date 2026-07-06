#pragma once

#include <cstdint>
#include <functional>

#include "local/logger.hpp"
#include "device/tca8418.hpp"

namespace wrapper
{

// ============================================================
// 键盘事件结构体
// ============================================================

/// @brief T-LoraPager 单次键盘事件（已完成模式解码）
struct LilyGoLoRaPagerKeyEvent
{
    bool pressed;  ///< true = 按下，false = 释放
    uint8_t code;  ///< TCA8418 原始 1-based 键码（1–40）
    uint8_t row;   ///< 物理矩阵行号（0–3）
    uint8_t col;   ///< 物理矩阵列号（0–9）

    /// @brief 当前模式下的可打印字符
    ///  - 普通字母（受 cap_mode 影响：小写/大写）
    ///  - SYM 模式下为符号/数字层
    ///  - 特殊键（ALT/CAPS）→ '\0'
    ///  - BS  → '\b'
    ///  - ENT → '\n'
    ///  - SYM/SPC → ' '
    char ch;

    const char* label;  ///< 可读键名（如 "Q", "SYM", "BS", "CAP", "ENT"）

    bool cap_mode;  ///< 本次事件处理后 CAPS 锁定状态
    bool sym_mode;  ///< 本次事件处理后 SYM 模式状态
};

/// @brief 键盘事件回调类型
using LilyGoLoRaPagerKeyCallback = std::function<void(const LilyGoLoRaPagerKeyEvent&)>;

// ============================================================
// T-LoraPager 高层键盘驱动
//
// 封装 Tca8418（4 行 × 10 列，40 键）并增加：
//   - 双层键值表（normal / symbol）
//   - CAPS 锁（对字母键大小写切换）
//   - SYM 模式（切换数字/符号层；SYM 键同时输出空格）
//   - 每次事件携带解码后的 char 和 label，可直接使用
//   - 可选事件回调，也可逐个读取事件
// ============================================================

class LilyGoLoRaPagerKeyboard
{
   public:
    // ---- 物理参数 ----
    static constexpr uint8_t kRows = 4;   ///< 矩阵行数
    static constexpr uint8_t kCols = 10;  ///< 矩阵列数

    // ---- 特殊键的 1-based 键码 ----
    //  布局：
    //    Row 0 ( 1-10): Q  W  E  R  T  Y  U  I  O  P
    //    Row 1 (11-20): A  S  D  F  G  H  J  K  L  ENT
    //    Row 2 (21-30): ALT Z  X  C  V  B  N  M  CAP BS
    //    Row 3 (31-40): SYM …
    static constexpr uint8_t kCodeAlt = 21;   ///< ALT  (row2, col0)
    static constexpr uint8_t kCodeCaps = 29;  ///< CAP  (row2, col8)
    static constexpr uint8_t kCodeBs = 30;    ///< BS   (row2, col9)
    static constexpr uint8_t kCodeSym = 31;   ///< SYM  (row3, col0) — 同时输出空格

   public:
    /// @param logger  日志对象引用
    /// @param tca     已初始化的 Tca8418 引用（生命期须长于本对象）
    explicit LilyGoLoRaPagerKeyboard(Logger& logger, Tca8418& tca);
    ~LilyGoLoRaPagerKeyboard() = default;

    // ---- 回调注册 ----

    /// @brief 注册键盘事件回调（传 nullptr 清除）
    void SetKeyCallback(LilyGoLoRaPagerKeyCallback callback);

    // ---- 主循环接口 ----

    /// @brief 轮询 TCA8418 FIFO，将所有待处理事件解码后依次触发回调
    /// @return 无 I2C 错误时返回 true；通信失败返回 false
    bool Poll();

    // ---- 单步接口（无需回调）----

    /// @brief 读取一条已解码事件
    /// @param event 输出的事件结构
    /// @return true = 成功读取；false = FIFO 为空或 I2C 错误
    bool GetKeyEvent(LilyGoLoRaPagerKeyEvent& event);

    // ---- 状态查询 ----

    bool IsCapMode() const { return cap_mode_; }  ///< 当前 CAPS 锁状态
    bool IsSymMode() const { return sym_mode_; }  ///< 当前 SYM 模式状态

    /// @brief 清除 cap_mode 和 sym_mode
    void ResetModes();

   private:
    Logger& logger_;
    Tca8418& tca_;
    LilyGoLoRaPagerKeyCallback callback_;

    bool cap_mode_{false};
    bool sym_mode_{false};

    /// @brief 从原始码 + pressed 生成完整事件（含模式更新）
    void DecodeEvent(uint8_t raw_code, bool pressed, LilyGoLoRaPagerKeyEvent& out);

    // ---- 静态键值表 ----

    /// @brief 普通模式字符表 [row][col]（来源：LilyGo_LoRa_Pager.cpp keymap）
    static const char kNormalMap[kRows][kCols];

    /// @brief 符号模式字符表 [row][col]（来源：LilyGo_LoRa_Pager.cpp symbol_map）
    static const char kSymbolMap[kRows][kCols];

    /// @brief 可读键名表（1-based 索引；0 = 无效）
    static const char* const kLabelTable[kRows * kCols + 1];

    /// @brief 根据键码和当前模式返回可打印字符
    static char ResolveChar(uint8_t code, bool cap_mode, bool sym_mode);
};

}  // namespace wrapper
