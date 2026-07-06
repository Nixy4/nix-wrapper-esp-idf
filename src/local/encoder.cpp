#include "local/encoder.hpp"

#include "driver/gpio.h"
#include "esp_attr.h"

namespace wrapper
{

// =============================================================================
// 全步进正交解码状态机
//
// 状态索引：当前状态 (0–6)
// 输入索引：(pin_b << 1) | pin_a  → 0–3
// 输出值  ：低 4 位 = 下一状态；高 2 位 = 方向标志
// =============================================================================

#define DIR_NONE 0x00u  // 无有效步进
#define DIR_CW 0x10u    // 顺时针步进
#define DIR_CCW 0x20u   // 逆时针步进

// 状态别名
static constexpr uint8_t kStart = 0;
static constexpr uint8_t kCwFinal = 1;
static constexpr uint8_t kCwBegin = 2;
static constexpr uint8_t kCwNext = 3;
static constexpr uint8_t kCcwBegin = 4;
static constexpr uint8_t kCcwFinal = 5;
static constexpr uint8_t kCcwNext = 6;

// 表格：ttable[state][AB] → next_state | dir_flag
static const uint8_t kTtable[7][4] = {
    // kStart (0)
    {kStart, kCwBegin, kCcwBegin, kStart},
    // kCwFinal (1)
    {kCwNext, kStart, kCwFinal, static_cast<uint8_t>(kStart | DIR_CW)},
    // kCwBegin (2)
    {kCwNext, kCwBegin, kStart, kStart},
    // kCwNext (3)
    {kCwNext, kCwBegin, kCwFinal, kStart},
    // kCcwBegin (4)
    {kCcwNext, kStart, kCcwBegin, kStart},
    // kCcwFinal (5)
    {kCcwNext, kCcwFinal, kStart, static_cast<uint8_t>(kStart | DIR_CCW)},
    // kCcwNext (6)
    {kCcwNext, kCcwFinal, kCcwBegin, kStart},
};

// =============================================================================
// ISR 处理函数
// =============================================================================

void Encoder::RotIsr(void* arg) { static_cast<Encoder*>(arg)->ProcessAB(); }

void Encoder::BtnIsr(void* arg)
{
    Encoder* self = static_cast<Encoder*>(arg);
    // 低电平 = 按下
    if (gpio_get_level(self->pin_btn_) == 0)
        self->clicked_.store(true, std::memory_order_relaxed);
}

void Encoder::ProcessAB()
{
    const uint8_t pin_a_val = static_cast<uint8_t>(gpio_get_level(pin_a_));
    const uint8_t pin_b_val = static_cast<uint8_t>(gpio_get_level(pin_b_));
    const uint8_t ab = static_cast<uint8_t>((pin_b_val << 1) | pin_a_val);

    const uint8_t entry = kTtable[state_ & 0x07u][ab];
    state_ = entry & 0x0Fu;

    const uint8_t dir = entry & 0x30u;
    if (dir == DIR_CW)
        delta_.fetch_add(1, std::memory_order_relaxed);
    else if (dir == DIR_CCW)
        delta_.fetch_add(-1, std::memory_order_relaxed);
}

// =============================================================================
// 公有接口
// =============================================================================

bool Encoder::Init(gpio_num_t pin_a, gpio_num_t pin_b, gpio_num_t pin_btn)
{
    pin_a_ = pin_a;
    pin_b_ = pin_b;
    pin_btn_ = pin_btn;
    state_ = 0;
    delta_.store(0);
    clicked_.store(false);

    // ── A/B 相：输入 + 内部上拉 + 任意边沿中断 ────────────────────
    gpio_config_t rot_cfg = {};
    rot_cfg.pin_bit_mask = (1ULL << pin_a) | (1ULL << pin_b);
    rot_cfg.mode = GPIO_MODE_INPUT;
    rot_cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    rot_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    rot_cfg.intr_type = GPIO_INTR_ANYEDGE;
    if (gpio_config(&rot_cfg) != ESP_OK)
        return false;

    // ── 按钮：输入 + 内部上拉 + 下降沿中断（低电平有效） ─────────
    if (pin_btn != GPIO_NUM_NC)
    {
        gpio_config_t btn_cfg = {};
        btn_cfg.pin_bit_mask = (1ULL << pin_btn);
        btn_cfg.mode = GPIO_MODE_INPUT;
        btn_cfg.pull_up_en = GPIO_PULLUP_ENABLE;
        btn_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
        btn_cfg.intr_type = GPIO_INTR_NEGEDGE;
        if (gpio_config(&btn_cfg) != ESP_OK)
            return false;
    }

    // ── 安装 GPIO ISR 服务（若已安装则忽略 ESP_ERR_INVALID_STATE） ─
    esp_err_t ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
        return false;

    // ── 注册 ISR 处理函数 ──────────────────────────────────────────
    gpio_isr_handler_add(pin_a, RotIsr, this);
    gpio_isr_handler_add(pin_b, RotIsr, this);
    if (pin_btn != GPIO_NUM_NC)
        gpio_isr_handler_add(pin_btn, BtnIsr, this);

    initialized_ = true;
    return true;
}

int32_t Encoder::GetAndClearDelta() { return delta_.exchange(0, std::memory_order_relaxed); }

bool Encoder::GetAndClearClick() { return clicked_.exchange(false, std::memory_order_relaxed); }

bool Encoder::IsButtonPressed() const
{
    if (pin_btn_ == GPIO_NUM_NC)
        return false;
    return gpio_get_level(pin_btn_) == 0;
}

}  // namespace wrapper
