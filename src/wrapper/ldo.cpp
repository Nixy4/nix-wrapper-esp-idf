#include "ldo.hpp"
#include "esp_err.h"

namespace wrapper
{

bool LdoRegulator::Init(const LdoChannelConfig &config)
{
    if (channel_handle_ != nullptr)
    {
        logger_.Warning("LDO channel already initialized");
        return false;
    }

    esp_err_t ret = esp_ldo_acquire_channel(&config, &channel_handle_);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to acquire LDO channel %d: %s", config.chan_id, esp_err_to_name(ret));
        channel_handle_ = nullptr;
        return false;
    }

    logger_.Info("LDO channel %d acquired successfully (voltage: %dmV)", config.chan_id,
                 config.voltage_mv);
    return true;
}

bool LdoRegulator::Deinit()
{
    if (channel_handle_ == nullptr)
    {
        return true;
    }

    esp_err_t ret = esp_ldo_release_channel(channel_handle_);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to release LDO channel: %s", esp_err_to_name(ret));
        return false;
    }

    logger_.Info("LDO channel released successfully");
    channel_handle_ = nullptr;
    return true;
}

bool LdoRegulator::AdjustVoltage(int voltage_mv)
{
    if (channel_handle_ == nullptr)
    {
        logger_.Error("LDO channel not initialized");
        return false;
    }

    esp_err_t ret = esp_ldo_channel_adjust_voltage(channel_handle_, voltage_mv);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to adjust LDO voltage to %dmV: %s", voltage_mv, esp_err_to_name(ret));
        return false;
    }

    logger_.Info("LDO voltage adjusted to %dmV", voltage_mv);
    return true;
}

}  // namespace wrapper
