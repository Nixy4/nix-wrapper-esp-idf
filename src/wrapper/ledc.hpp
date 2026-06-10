#pragma once
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "wrapper/logger.hpp"

namespace wrapper
{
struct LedcTimerConfig : public ledc_timer_config_t
{
    LedcTimerConfig(ledc_mode_t speed_mode,
                    ledc_timer_bit_t duty_resolution,
                    ledc_timer_t timer_num,
                    uint32_t freq_hz,
                    ledc_clk_cfg_t clk_cfg,
                    bool deconfigure = false)
        : ledc_timer_config_t{}
    {
        this->speed_mode = speed_mode;
        this->duty_resolution = duty_resolution;
        this->timer_num = timer_num;
        this->freq_hz = freq_hz;
        this->clk_cfg = clk_cfg;
        this->deconfigure = deconfigure;
    }
};

struct LedcChannelConfig : public ledc_channel_config_t
{
    LedcChannelConfig(gpio_num_t gpio_num,
                      ledc_mode_t speed_mode,
                      ledc_channel_t channel,
                      ledc_timer_t timer_sel,
                      uint32_t duty,
                      int hpoint)
        : ledc_channel_config_t{}
    {
        this->gpio_num = gpio_num;
        this->speed_mode = speed_mode;
        this->channel = channel;
        this->timer_sel = timer_sel;
        this->duty = duty;
        this->hpoint = hpoint;
    }
};

class LedcTimer
{
   private:
    Logger& logger_;
    ledc_mode_t speed_mode_;
    ledc_timer_t timer_num_;
    bool initialized_;

   public:
    LedcTimer(Logger& logger);
    ~LedcTimer();
    ledc_mode_t GetSpeedMode() const { return speed_mode_; }
    ledc_timer_t GetTimerNum() const { return timer_num_; }
    bool IsInitialized() const { return initialized_; }
    // operations
    bool Init(const LedcTimerConfig& config);
    bool Deinit();
    bool Pause();
    bool Resume();
    bool SetFreq(uint32_t freq_hz);
};

class LedcChannel
{
   private:
    Logger& logger_;
    ledc_mode_t speed_mode_;
    ledc_channel_t channel_;
    bool initialized_;

   public:
    LedcChannel(Logger& logger);
    ~LedcChannel();
    ledc_mode_t GetSpeedMode() const { return speed_mode_; }
    ledc_channel_t GetChannel() const { return channel_; }
    bool IsInitialized() const { return initialized_; }
    // operations
    bool Init(const LedcChannelConfig& config);
    bool Deinit();
    bool SetDuty(uint32_t duty);
    bool SetDutyAndUpdate(uint32_t duty);
    bool UpdateDuty();
    bool Stop(uint32_t idle_level);
};
}  // namespace wrapper