#include "lilygo_t_lora_pager_keyboard.hpp"

#include <cctype>

namespace wrapper
{

// ============================================================
// 静态键值表（来源：.reference/LilyGoLib/src/LilyGo_LoRa_Pager.cpp）
// ============================================================

// clang-format off
const char LilyGoLoRaPagerKeyboard::kNormalMap[kRows][kCols] = {
    { 'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  'o',  'p'  },  // row0  1-10
    { 'a',  's',  'd',  'f',  'g',  'h',  'j',  'k',  'l',  '\n' },  // row1 11-20  (\n=ENT)
    { '\0', 'z',  'x',  'c',  'v',  'b',  'n',  'm',  '\0', '\0' },  // row2 21-30  (ALT/CAP/BS)
    { ' ',  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'  },  // row3 31-40  (SYM/SPC)
};

const char LilyGoLoRaPagerKeyboard::kSymbolMap[kRows][kCols] = {
    { '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0'  },  // row0
    { '*',  '/',  '+',  '-',  '=',  ':',  '\'', '"',  '@',  '\0' },  // row1
    { '\0', '_',  '$',  ';',  '?',  '!',  ',',  '.',  '\0', '\0' },  // row2
    { ' ',  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'  },  // row3
};

// 1-based; index 0 = invalid
const char* const LilyGoLoRaPagerKeyboard::kLabelTable[kRows * kCols + 1] = {
    "?",                                                      // 0  invalid
    "Q",  "W",  "E",  "R",  "T",  "Y",  "U",  "I",  "O",  "P",   //  1-10  row0
    "A",  "S",  "D",  "F",  "G",  "H",  "J",  "K",  "L",  "ENT", // 11-20  row1
    "ALT","Z",  "X",  "C",  "V",  "B",  "N",  "M",  "CAP","BS",  // 21-30  row2
    "SYM","?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",  "?",   // 31-40  row3
};
// clang-format on

// ============================================================
// 构造 / 配置
// ============================================================

LilyGoLoRaPagerKeyboard::LilyGoLoRaPagerKeyboard(Logger& logger, Tca8418& tca)
    : logger_(logger), tca_(tca), callback_(nullptr), cap_mode_(false), sym_mode_(false)
{
}

void LilyGoLoRaPagerKeyboard::SetKeyCallback(LilyGoLoRaPagerKeyCallback callback)
{
    callback_ = std::move(callback);
}

void LilyGoLoRaPagerKeyboard::ResetModes()
{
    cap_mode_ = false;
    sym_mode_ = false;
}

// ============================================================
// 事件轮询
// ============================================================

bool LilyGoLoRaPagerKeyboard::Poll()
{
    if (!tca_.EventAvailable())
        return true;

    uint8_t raw_code = 0;
    bool pressed = false;

    while (tca_.GetKeyEvent(raw_code, pressed))
    {
        LilyGoLoRaPagerKeyEvent ev{};
        DecodeEvent(raw_code, pressed, ev);
        if (callback_)
            callback_(ev);
    }
    return true;
}

bool LilyGoLoRaPagerKeyboard::GetKeyEvent(LilyGoLoRaPagerKeyEvent& event)
{
    uint8_t raw_code = 0;
    bool pressed = false;

    if (!tca_.GetKeyEvent(raw_code, pressed))
        return false;

    DecodeEvent(raw_code, pressed, event);
    return true;
}

// ============================================================
// 解码逻辑
// ============================================================

void LilyGoLoRaPagerKeyboard::DecodeEvent(uint8_t raw_code,
                                          bool pressed,
                                          LilyGoLoRaPagerKeyEvent& out)
{
    // ── 更新模式状态（仅在按下时切换）──────────────────────
    if (pressed)
    {
        if (raw_code == kCodeCaps)
            cap_mode_ = !cap_mode_;
        if (raw_code == kCodeSym)
            sym_mode_ = !sym_mode_;
    }

    // ── 填充事件字段 ────────────────────────────────────────
    out.pressed = pressed;
    out.code = raw_code;
    out.row = (raw_code > 0 && raw_code <= kRows * kCols)
                  ? static_cast<uint8_t>((raw_code - 1) / kCols)
                  : 0xFF;
    out.col = (raw_code > 0 && raw_code <= kRows * kCols)
                  ? static_cast<uint8_t>((raw_code - 1) % kCols)
                  : 0xFF;
    out.cap_mode = cap_mode_;
    out.sym_mode = sym_mode_;
    out.label = (raw_code <= kRows * kCols) ? kLabelTable[raw_code] : "?";
    out.ch = ResolveChar(raw_code, cap_mode_, sym_mode_);
}

// ============================================================
// 字符解析
// ============================================================

char LilyGoLoRaPagerKeyboard::ResolveChar(uint8_t code, bool cap_mode, bool sym_mode)
{
    if (code == 0 || code > kRows * kCols)
        return '\0';

    // 特殊功能键：无可打印字符输出
    if (code == kCodeAlt || code == kCodeCaps)
        return '\0';

    // BS 固定返回退格
    if (code == kCodeBs)
        return '\b';

    const uint8_t row = static_cast<uint8_t>((code - 1) / kCols);
    const uint8_t col = static_cast<uint8_t>((code - 1) % kCols);

    // SYM 模式：从符号表取字符
    if (sym_mode)
        return kSymbolMap[row][col];

    // 普通模式：取字母后按 cap_mode 转换大小写
    char c = kNormalMap[row][col];
    if (cap_mode && c >= 'a' && c <= 'z')
        c = static_cast<char>(toupper(static_cast<unsigned char>(c)));

    return c;
}

}  // namespace wrapper
