#include "wrapper/touch.hpp"

using namespace wrapper;

I2cTouch::I2cTouch(Logger &logger) : logger_(logger), io_handle_(NULL), touch_handle_(NULL) {}

I2cTouch::~I2cTouch() { Deinit(); }

bool I2cTouch::Init(
    const I2cBus &bus, const I2cTouchConfig &config,
    std::function<esp_err_t(const esp_lcd_panel_io_handle_t, const esp_lcd_touch_config_t *,
                            esp_lcd_touch_handle_t *)>
        new_touch_func)
{
    if (io_handle_ != NULL || touch_handle_ != NULL)
    {
        logger_.Warning("Touch already initialized. Deinitializing first.");
        Deinit();
    }

    if (bus.GetHandle() == NULL)
    {
        logger_.Error("Bus not initialized");
        return false;
    }

    // 1. Create IO handle
    esp_err_t ret = esp_lcd_new_panel_io_i2c(bus.GetHandle(), &config.io_config, &io_handle_);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to create Touch IO handle: %s", esp_err_to_name(ret));
        return false;
    }

    // 2. Create Touch handle
    // Use the provided factory function to create the specific touch controller handle
    ret = new_touch_func(io_handle_, &config.touch_config, &touch_handle_);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to create Touch handle: %s", esp_err_to_name(ret));
        esp_lcd_panel_io_del(io_handle_);
        io_handle_ = NULL;
        return false;
    }

    logger_.Info("Touch initialized (Addr: 0x%02X)", config.io_config.dev_addr);
    return true;
}

bool I2cTouch::Deinit()
{
    esp_err_t ret = ESP_OK;
    if (touch_handle_ != NULL)
    {
        esp_err_t r = esp_lcd_touch_del(touch_handle_);
        if (r != ESP_OK)
        {
            logger_.Error("Failed to delete touch handle: %s", esp_err_to_name(r));
            ret = r;
        }
        touch_handle_ = NULL;
    }

    if (io_handle_ != NULL)
    {
        esp_err_t r = esp_lcd_panel_io_del(io_handle_);
        if (r != ESP_OK)
        {
            logger_.Error("Failed to delete IO handle: %s", esp_err_to_name(r));
            if (ret == ESP_OK)
                ret = r;
        }
        io_handle_ = NULL;
    }

    if (ret == ESP_OK)
    {
        logger_.Info("Touch deinitialized");
        return true;
    }
    return false;
}

bool I2cTouch::ReadData()
{
    if (touch_handle_ == NULL)
    {
        return false;
    }
    return esp_lcd_touch_read_data(touch_handle_) == ESP_OK;
}

bool I2cTouch::GetData(esp_lcd_touch_point_data_t *data, uint8_t *point_cnt, uint8_t max_point_cnt)
{
    if (touch_handle_ == NULL)
    {
        return false;
    }
    return esp_lcd_touch_get_data(touch_handle_, data, point_cnt, max_point_cnt) == ESP_OK;
}

bool I2cTouch::GetCoordinates(uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num,
                              uint8_t max_point_num)
{
    if (touch_handle_ == NULL)
    {
        return false;
    }

    // Use new API and convert to old format for backward compatibility
    esp_lcd_touch_point_data_t data[max_point_num];
    uint8_t cnt = 0;
    esp_err_t ret = esp_lcd_touch_get_data(touch_handle_, data, &cnt, max_point_num);

    if (ret != ESP_OK)
    {
        return false;
    }

    if (point_num != NULL)
    {
        *point_num = cnt;
    }

    for (uint8_t i = 0; i < cnt; i++)
    {
        if (x != NULL)
            x[i] = data[i].x;
        if (y != NULL)
            y[i] = data[i].y;
        if (strength != NULL)
            strength[i] = data[i].strength;
    }

    return (cnt > 0);
}
