#include "wrapper/display.hpp"

using namespace wrapper;

bool I2cDisplay::InitIo(const I2cBus& bus, const I2cDisplayConfig& config)
{
    if (io_handle_ != nullptr)
        return true;

    esp_err_t err = esp_lcd_new_panel_io_i2c(bus.GetHandle(), &config.io_config, &io_handle_);
    if (err != ESP_OK)
    {
        logger_.Error("Failed to create I2C panel IO: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool I2cDisplay::InitPanel(
    const I2cDisplayConfig& config,
    std::function<esp_err_t(const esp_lcd_panel_io_handle_t)> custom_init_panel_func)
{
    esp_err_t err = ESP_OK;

    if (custom_init_panel_func != nullptr)
    {
        err = custom_init_panel_func(io_handle_);
        if (err != ESP_OK)
        {
            logger_.Error("Failed to custom init panel: %s", esp_err_to_name(err));
            return false;
        }
        logger_.Info("Custom panel initialization completed successfully");
    }
    else
    {
        err = esp_lcd_panel_init(panel_handle_);
        if (err != ESP_OK)
        {
            logger_.Error("Failed to init panel: %s", esp_err_to_name(err));
            return false;
        }
        logger_.Info("Panel initialized successfully wit Default settings");
    }

    err = esp_lcd_panel_reset(panel_handle_);
    if (err != ESP_OK)
    {
        logger_.Error("Failed to reset panel: %s", esp_err_to_name(err));
        return false;
    }

    logger_.Info("Display panel initialized successfully");
    return true;
}

bool I2cDisplay::Init(
    const I2cBus& bus,
    const I2cDisplayConfig& config,
    std::function<esp_err_t(const esp_lcd_panel_io_handle_t,
                            const esp_lcd_panel_dev_config_t*,
                            esp_lcd_panel_handle_t*)> new_panel_func,
    std::function<esp_err_t(const esp_lcd_panel_io_handle_t)> custom_init_panel_func)
{
    if (!InitIo(bus, config))
        return false;

    esp_err_t err = new_panel_func(io_handle_, &config.panel_config, &panel_handle_);
    if (err != ESP_OK)
    {
        logger_.Error("Failed to create new panel: %s", esp_err_to_name(err));
        return false;
    }

    return InitPanel(config, custom_init_panel_func);
}

bool I2cDisplay::Deinit()
{
    if (panel_handle_ != nullptr)
    {
        if (esp_lcd_panel_del(panel_handle_) != ESP_OK)
        {
            logger_.Error("Failed to delete panel");
            return false;
        }
        panel_handle_ = nullptr;
    }

    if (io_handle_ != nullptr)
    {
        if (esp_lcd_panel_io_del(io_handle_) != ESP_OK)
        {
            logger_.Error("Failed to delete panel IO");
            return false;
        }
        io_handle_ = nullptr;
    }
    return true;
}

bool SpiDisplay::InitIo(const SpiBus& bus, const SpiDisplayConfig& config)
{
    if (io_handle_ != nullptr)
        return true;

    esp_err_t err = esp_lcd_new_panel_io_spi(bus.GetHostId(), &config.io_config, &io_handle_);
    if (err != ESP_OK)
    {
        logger_.Error("Failed to create SPI panel IO: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool SpiDisplay::InitPanel(
    const SpiDisplayConfig& config,
    std::function<esp_err_t(const esp_lcd_panel_io_handle_t)> custom_init_panel_func)
{
    esp_err_t err = ESP_OK;

    if (custom_init_panel_func != nullptr)
    {
        err = custom_init_panel_func(io_handle_);
        if (err != ESP_OK)
        {
            logger_.Error("Failed to custom init panel: %s", esp_err_to_name(err));
            return false;
        }
    }
    else
    {
        err = esp_lcd_panel_init(panel_handle_);
        if (err != ESP_OK)
        {
            logger_.Error("Failed to init panel: %s", esp_err_to_name(err));
            return false;
        }
    }

    err = esp_lcd_panel_reset(panel_handle_);
    if (err != ESP_OK)
    {
        logger_.Error("Failed to reset panel: %s", esp_err_to_name(err));
        return false;
    }

    return true;
}

bool SpiDisplay::Init(
    const SpiBus& bus,
    const SpiDisplayConfig& config,
    std::function<esp_err_t(const esp_lcd_panel_io_handle_t,
                            const esp_lcd_panel_dev_config_t*,
                            esp_lcd_panel_handle_t*)> new_panel_func,
    std::function<esp_err_t(const esp_lcd_panel_io_handle_t)> custom_init_panel_func)
{
    if (!InitIo(bus, config))
        return false;

    esp_err_t err = new_panel_func(io_handle_, &config.panel_config, &panel_handle_);
    if (err != ESP_OK)
    {
        logger_.Error("Failed to create new panel: %s", esp_err_to_name(err));
        return false;
    }

    return InitPanel(config, custom_init_panel_func);
}

bool SpiDisplay::Deinit()
{
    if (panel_handle_ != nullptr)
    {
        if (esp_lcd_panel_del(panel_handle_) != ESP_OK)
        {
            logger_.Error("Failed to delete panel");
            return false;
        }
        panel_handle_ = nullptr;
    }

    if (io_handle_ != nullptr)
    {
        if (esp_lcd_panel_io_del(io_handle_) != ESP_OK)
        {
            logger_.Error("Failed to delete panel IO");
            return false;
        }
        io_handle_ = nullptr;
    }
    return true;
}
