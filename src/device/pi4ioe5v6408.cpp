#include "pi4ioe5v6408.hpp"

#if __has_include("esp_io_expander.h")

namespace wrapper
{

Pi4ioe5v6408::Pi4ioe5v6408(Logger& logger) : m_logger(logger) {}

Pi4ioe5v6408::~Pi4ioe5v6408() { Deinit(); }

bool Pi4ioe5v6408::Init(const I2cBus& bus, uint8_t dev_addr)
{
    if (m_handle != nullptr)
    {
        m_logger.Warning("PI4IOE5V6408 already initialized");
        return true;
    }

    if (bus.GetHandle() == nullptr)
    {
        m_logger.Error("I2C bus not initialized");
        return false;
    }

    esp_err_t ret = esp_io_expander_new_i2c_pi4ioe5v6408(bus.GetHandle(), dev_addr, &m_handle);
    if (ret != ESP_OK)
    {
        m_logger.Error("Failed to create PI4IOE5V6408 IO expander: %s", esp_err_to_name(ret));
        return false;
    }

    m_logger.Info("PI4IOE5V6408 IO expander initialized (addr: 0x%02x)", dev_addr);
    return true;
}

bool Pi4ioe5v6408::Deinit()
{
    if (m_handle != nullptr)
    {
        esp_err_t ret = esp_io_expander_del(m_handle);
        if (ret != ESP_OK)
        {
            m_logger.Error("Failed to delete IO expander: %s", esp_err_to_name(ret));
            return false;
        }
        m_handle = nullptr;
        m_logger.Info("PI4IOE5V6408 IO expander deinitialized");
    }
    return true;
}

bool Pi4ioe5v6408::SetDirection(uint32_t io_num, uint32_t direction)
{
    if (m_handle == nullptr)
    {
        m_logger.Error("IO expander not initialized");
        return false;
    }

    esp_err_t ret = esp_io_expander_set_dir(m_handle, io_num, (esp_io_expander_dir_t)direction);
    if (ret != ESP_OK)
    {
        m_logger.Error("Failed to set direction: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool Pi4ioe5v6408::SetLevel(uint32_t io_num, uint32_t level)
{
    if (m_handle == nullptr)
    {
        m_logger.Error("IO expander not initialized");
        return false;
    }

    esp_err_t ret = esp_io_expander_set_level(m_handle, io_num, level);
    if (ret != ESP_OK)
    {
        m_logger.Error("Failed to set level: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool Pi4ioe5v6408::GetLevel(uint32_t io_num, uint32_t* level)
{
    if (m_handle == nullptr)
    {
        m_logger.Error("IO expander not initialized");
        return false;
    }

    esp_err_t ret = esp_io_expander_get_level(m_handle, io_num, level);
    if (ret != ESP_OK)
    {
        m_logger.Error("Failed to get level: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool Pi4ioe5v6408::SetPullupMode(uint32_t io_num, uint32_t pull_mode)
{
    if (m_handle == nullptr)
    {
        m_logger.Error("IO expander not initialized");
        return false;
    }

    esp_err_t ret =
        esp_io_expander_set_pullupdown(m_handle, io_num, (esp_io_expander_pullupdown_t)pull_mode);
    if (ret != ESP_OK)
    {
        m_logger.Error("Failed to set pullup mode: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool Pi4ioe5v6408::SetOutputMode(uint32_t io_num, esp_io_expander_output_mode_t mode)
{
    if (m_handle == nullptr)
    {
        m_logger.Error("IO expander not initialized");
        return false;
    }

    esp_err_t ret = esp_io_expander_set_output_mode(m_handle, io_num, mode);
    if (ret != ESP_OK)
    {
        m_logger.Error("Failed to set output mode: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool Pi4ioe5v6408::PrintState()
{
    if (m_handle == nullptr)
    {
        m_logger.Error("IO expander not initialized");
        return false;
    }

    return esp_io_expander_print_state(m_handle) == ESP_OK;
}

}  // namespace wrapper

#endif  // __has_include("esp_io_expander.h")