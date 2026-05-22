#pragma once

#include "esp_ldo_regulator.h"
#include "logger.hpp"

namespace wrapper
{
struct LdoChannelConfig : public esp_ldo_channel_config_t
{
    LdoChannelConfig(int chan_id, int voltage_mv, bool adjustable = false, bool owned_by_hw = false)
        : esp_ldo_channel_config_t{}
    {
        this->chan_id = chan_id;
        this->voltage_mv = voltage_mv;
        this->flags.adjustable = adjustable ? 1U : 0U;
        this->flags.owned_by_hw = owned_by_hw ? 1U : 0U;
    }
};

class LdoRegulator
{
   private:
    Logger &logger_;
    esp_ldo_channel_handle_t channel_handle_;

   public:
    LdoRegulator(Logger &logger) : logger_(logger), channel_handle_(nullptr) {}
    ~LdoRegulator() { Deinit(); }
    // operations
    bool Init(const LdoChannelConfig &config);
    bool Deinit();
    bool AdjustVoltage(int voltage_mv);
    esp_ldo_channel_handle_t GetHandle() const { return channel_handle_; }
};
}  // namespace wrapper