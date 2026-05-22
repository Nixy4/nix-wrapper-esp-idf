#include "wrapper/display-dsi.hpp"

namespace wrapper
{

// --- DsiBus ---

DsiBus::DsiBus(Logger& logger) : logger_(logger) {}

DsiBus::~DsiBus() { Deinit(); }

Logger& DsiBus::GetLogger() { return logger_; }

esp_lcd_dsi_bus_handle_t DsiBus::GetHandle() const { return bus_handle_; }

bool DsiBus::Init(const DsiBusConfig& config)
{
    if (bus_handle_ != nullptr)
    {
        logger_.Warning("Already initialized. Deinitializing first.");
        Deinit();
    }

    esp_err_t ret = esp_lcd_new_dsi_bus(&config, &bus_handle_);
    if (ret == ESP_OK)
    {
        logger_.Info("Initialized (Bus ID: %d, Lanes: %d, Rate: %lu Mbps)", config.bus_id,
                     config.num_data_lanes, config.lane_bit_rate_mbps);
        return true;
    }
    else
    {
        logger_.Error("Failed to initialize: %s", esp_err_to_name(ret));
        return false;
    }
}

bool DsiBus::Deinit()
{
    if (bus_handle_ != nullptr)
    {
        esp_err_t ret = esp_lcd_del_dsi_bus(bus_handle_);
        if (ret == ESP_OK)
        {
            logger_.Info("Deinitialized");
            bus_handle_ = nullptr;
            return true;
        }
        else
        {
            logger_.Error("Failed to deinitialize: %s", esp_err_to_name(ret));
            return false;
        }
    }
    return true;
}

// --- DsiDisplay ---

DsiDisplay::DsiDisplay(Logger& logger) : DsiDisplay::DisplayBase(logger) {}

DsiDisplay::~DsiDisplay() { Deinit(); }

bool DsiDisplay::Init(
    const DsiBus& bus, const DsiDisplayConfig& config,
    std::function<esp_err_t(const esp_lcd_panel_io_handle_t, const esp_lcd_panel_dev_config_t*,
                            esp_lcd_panel_handle_t*)>
        new_panel_func,
    std::function<esp_err_t(const esp_lcd_panel_io_handle_t)> custom_init_panel_func,
    void* vendor_config, std::function<void(void)> vendor_config_init_func)
{
    // 1. Initialize Panel IO (DBI)
    if (!InitIo(bus, config.dbi_config))
        return false;

    // 2. Execute vendor config initialization callback if provided
    if (vendor_config_init_func)
    {
        vendor_config_init_func();
    }

    // 3. Create Panel handle with vendor_config
    esp_lcd_panel_dev_config_t panel_config = config.panel_config;
    panel_config.vendor_config = vendor_config;

    esp_err_t ret = new_panel_func(io_handle_, &panel_config, &panel_handle_);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to create LCD panel handle: %s", esp_err_to_name(ret));
        return false;
    }

    // 4. Initialize Panel (reset, init, turn on)
    return InitPanel(panel_config, custom_init_panel_func, vendor_config, vendor_config_init_func);
}

bool DsiDisplay::InitIo(const DsiBus& bus, const esp_lcd_dbi_io_config_t& config)
{
    if (io_handle_ != nullptr)
    {
        logger_.Warning("Display IO already initialized. Deinitializing first.");
        Deinit();
    }

    if (bus.GetHandle() == nullptr)
    {
        logger_.Error("DSI bus not initialized");
        return false;
    }

    // Create DBI IO handle
    esp_err_t ret = esp_lcd_new_panel_io_dbi(bus.GetHandle(), &config, &io_handle_);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to create LCD DBI IO handle: %s", esp_err_to_name(ret));
        return false;
    }

    logger_.Info("LCD DBI IO initialized (VC: %d)", config.virtual_channel);
    return true;
}

bool DsiDisplay::InitPanel(
    const esp_lcd_panel_dev_config_t& panel_config,
    std::function<esp_err_t(const esp_lcd_panel_io_handle_t)> custom_init_panel_func,
    void* vendor_config, std::function<void(void)> vendor_config_init_func)
{
    if (io_handle_ == nullptr)
    {
        logger_.Error("IO handle not initialized. Call Init first.");
        return false;
    }

    if (panel_handle_ == nullptr)
    {
        logger_.Error("Panel handle not created. Create it before calling InitPanel.");
        return false;
    }

    // 1. Reset the display
    logger_.Info("Resetting panel...");
    esp_err_t ret = esp_lcd_panel_reset(panel_handle_);
    if (ret != ESP_OK)
    {
        logger_.Error("Panel Reset Failed: %s", esp_err_to_name(ret));
        return false;
    }

    // 2. Initialize the display
    logger_.Info("Initializing panel...");
    if (custom_init_panel_func == nullptr)
    {
        ret = esp_lcd_panel_init(panel_handle_);
    }
    else
    {
        ret = custom_init_panel_func(io_handle_);
    }

    if (ret != ESP_OK)
    {
        logger_.Error("Panel Init Failed: %s", esp_err_to_name(ret));
        return false;
    }

    // 3. Set invert color (optional, typically false for most panels)
    logger_.Info("Setting invert color...");
    ret = esp_lcd_panel_invert_color(panel_handle_, false);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to set invert color: %s", esp_err_to_name(ret));
        return false;
    }

    logger_.Info("Display initialization complete!");
    return true;
}

bool DsiDisplay::Deinit()
{
    esp_err_t ret = ESP_OK;

    if (panel_handle_ != nullptr)
    {
        esp_err_t r = esp_lcd_panel_del(panel_handle_);
        if (r != ESP_OK)
        {
            logger_.Error("Failed to delete panel handle: %s", esp_err_to_name(r));
            ret = r;
        }
        panel_handle_ = nullptr;
    }

    if (io_handle_ != nullptr)
    {
        esp_err_t r = esp_lcd_panel_io_del(io_handle_);
        if (r != ESP_OK)
        {
            logger_.Error("Failed to delete IO handle: %s", esp_err_to_name(r));
            if (ret == ESP_OK)
                ret = r;
        }
        io_handle_ = nullptr;
    }

    if (ret == ESP_OK)
    {
        logger_.Info("DSI Display deinitialized");
        return true;
    }

    return false;
}

}  // namespace wrapper
