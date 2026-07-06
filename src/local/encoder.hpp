#pragma once

#include <atomic>
#include "driver/gpio.h"
#include "local/logger.hpp"

namespace wrapper
{

/**
 * @brief 基于 GPIO 中断的正交旋转编码器驱动。
 *
 * 使用全步进正交解码状态机处理 A/B 两相信号，
 * 并通过 GPIO 边沿中断驱动。
 *
 * 支持可选按钮引脚（下降沿触发，内部上拉，低电平有效）。
 *
 * 线程安全：delta 和 click 标志均通过 std::atomic 访问，
 * 可从任意任务安全读取。
 */
class Encoder
{
   public:
    Encoder() = default;
    ~Encoder() = default;

    // 禁止拷贝（ISR 持有 this 指针）
    Encoder(const Encoder&) = delete;
    Encoder& operator=(const Encoder&) = delete;

    /**
     * @brief 初始化编码器 GPIO 并安装中断处理函数。
     *
     * @param pin_a    A 相 GPIO 引脚号
     * @param pin_b    B 相 GPIO 引脚号
     * @param pin_btn  按钮 GPIO 引脚号（GPIO_NUM_NC 表示无按钮）
     * @return true 成功；false 失败（GPIO 配置错误）
     */
    bool Init(gpio_num_t pin_a, gpio_num_t pin_b, gpio_num_t pin_btn = GPIO_NUM_NC);

    /**
     * @brief 读取并清除自上次调用以来的累积步数。
     *
     * @return 步数（正值 = 顺时针，负值 = 逆时针）
     */
    int32_t GetAndClearDelta();

    /**
     * @brief 读取并清除按钮点击标志。
     *
     * @return true 表示自上次调用后按钮被按下过一次
     */
    bool GetAndClearClick();

    /**
     * @brief 查询按钮当前状态。
     *
     * @return true = 按钮当前被按下（低电平）
     */
    bool IsButtonPressed() const;

    bool IsInitialized() const { return initialized_; }

   private:
    gpio_num_t pin_a_ = GPIO_NUM_NC;
    gpio_num_t pin_b_ = GPIO_NUM_NC;
    gpio_num_t pin_btn_ = GPIO_NUM_NC;
    bool initialized_ = false;

    std::atomic<int32_t> delta_{0};
    std::atomic<bool> clicked_{false};

    // 全步进状态机当前状态（0-6）
    volatile uint8_t state_ = 0;

    // GPIO ISR 处理函数（IRAM_ATTR 确保在 cache 被禁用时可调用）
    static void IRAM_ATTR RotIsr(void* arg);
    static void IRAM_ATTR BtnIsr(void* arg);

    // 在 ISR 中调用：读取 A/B 电平，更新状态机，累加 delta
    void IRAM_ATTR ProcessAB();
};

}  // namespace wrapper
