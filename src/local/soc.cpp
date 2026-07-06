#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "local/soc.hpp"

using namespace wrapper;

// Nvs Implementation

Nvs::Nvs(Logger& logger) : logger_(logger), nvs_handle_(0) {}

Nvs::~Nvs()
{
    if (nvs_handle_ != 0)
    {
        nvs_close(nvs_handle_);
    }
}

bool Nvs::Init()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        logger_.Warning("partition was truncated and needs to be erased");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK)
    {
        logger_.Error("Failed to initialize flash");
        return false;
    }
    logger_.Info("flash initialized successfully");
    return true;
}

bool Nvs::OpenNamespace(std::string_view namespace_name, nvs_open_mode_t open_mode)
{
    if (nvs_handle_ != 0)
    {
        nvs_close(nvs_handle_);
        nvs_handle_ = 0;
    }

    esp_err_t err = nvs_open(namespace_name.data(), open_mode, &nvs_handle_);
    if (err != ESP_OK)
    {
        logger_.Error("Failed to open namespace: %s", namespace_name.data());
        return false;
    }
    logger_.Info("Opened namespace: %s", namespace_name.data());
    return true;
}

bool Nvs::Erase()
{
    if (nvs_handle_ == 0)
    {
        logger_.Error("namespace not opened");
        return false;
    }
    esp_err_t err = nvs_erase_all(nvs_handle_);
    if (err != ESP_OK)
    {
        logger_.Error("Failed to erase namespace");
        return false;
    }
    logger_.Info("namespace erased successfully");
    return true;
}

bool Nvs::Commit()
{
    if (nvs_handle_ == 0)
    {
        logger_.Error("namespace not opened");
        return false;
    }
    esp_err_t err = nvs_commit(nvs_handle_);
    if (err != ESP_OK)
    {
        logger_.Error("Failed to commit changes");
        return false;
    }
    logger_.Debug("changes committed");
    return true;
}

bool Nvs::EraseKey(std::string_view key)
{
    if (nvs_handle_ == 0)
    {
        logger_.Error("namespace not opened");
        return false;
    }
    esp_err_t err = nvs_erase_key(nvs_handle_, key.data());
    if (err != ESP_OK)
    {
        logger_.Error("Failed to erase key: %s", key.data());
        return false;
    }
    logger_.Debug("key erased: %s", key.data());
    return true;
}

template <typename T>
bool Nvs::SetValue(std::string_view key, T value)
{
    if (nvs_handle_ == 0)
    {
        logger_.Error("namespace not opened");
        return false;
    }

    esp_err_t err;
    if constexpr (std::is_same_v<T, uint8_t>)
    {
        err = nvs_set_u8(nvs_handle_, key.data(), value);
    }
    else if constexpr (std::is_same_v<T, int8_t>)
    {
        err = nvs_set_i8(nvs_handle_, key.data(), value);
    }
    else if constexpr (std::is_same_v<T, uint16_t>)
    {
        err = nvs_set_u16(nvs_handle_, key.data(), value);
    }
    else if constexpr (std::is_same_v<T, int16_t>)
    {
        err = nvs_set_i16(nvs_handle_, key.data(), value);
    }
    else if constexpr (std::is_same_v<T, uint32_t>)
    {
        err = nvs_set_u32(nvs_handle_, key.data(), value);
    }
    else if constexpr (std::is_same_v<T, int32_t>)
    {
        err = nvs_set_i32(nvs_handle_, key.data(), value);
    }
    else if constexpr (std::is_same_v<T, uint64_t>)
    {
        err = nvs_set_u64(nvs_handle_, key.data(), value);
    }
    else if constexpr (std::is_same_v<T, int64_t>)
    {
        err = nvs_set_i64(nvs_handle_, key.data(), value);
    }
    else
    {
        logger_.Error("Unsupported type for SetValue");
        return false;
    }

    if (err != ESP_OK)
    {
        logger_.Error("Failed to set value for key: %s", key.data());
        return false;
    }
    logger_.Debug("value set for key: %s", key.data());
    return true;
}

template <typename T>
bool Nvs::GetValue(std::string_view key, T& out_value)
{
    if (nvs_handle_ == 0)
    {
        logger_.Error("namespace not opened");
        return false;
    }

    esp_err_t err;
    if constexpr (std::is_same_v<T, uint8_t>)
    {
        err = nvs_get_u8(nvs_handle_, key.data(), &out_value);
    }
    else if constexpr (std::is_same_v<T, int8_t>)
    {
        err = nvs_get_i8(nvs_handle_, key.data(), &out_value);
    }
    else if constexpr (std::is_same_v<T, uint16_t>)
    {
        err = nvs_get_u16(nvs_handle_, key.data(), &out_value);
    }
    else if constexpr (std::is_same_v<T, int16_t>)
    {
        err = nvs_get_i16(nvs_handle_, key.data(), &out_value);
    }
    else if constexpr (std::is_same_v<T, uint32_t>)
    {
        err = nvs_get_u32(nvs_handle_, key.data(), &out_value);
    }
    else if constexpr (std::is_same_v<T, int32_t>)
    {
        err = nvs_get_i32(nvs_handle_, key.data(), &out_value);
    }
    else if constexpr (std::is_same_v<T, uint64_t>)
    {
        err = nvs_get_u64(nvs_handle_, key.data(), &out_value);
    }
    else if constexpr (std::is_same_v<T, int64_t>)
    {
        err = nvs_get_i64(nvs_handle_, key.data(), &out_value);
    }
    else
    {
        logger_.Error("Unsupported type for GetValue");
        return false;
    }

    if (err != ESP_OK)
    {
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            logger_.Debug("key not found: %s", key.data());
        }
        else
        {
            logger_.Error("Failed to get value for key: %s", key.data());
        }
        return false;
    }
    logger_.Debug("value retrieved for key: %s", key.data());
    return true;
}

bool Nvs::SetString(std::string_view key, std::string value)
{
    if (nvs_handle_ == 0)
    {
        logger_.Error("namespace not opened");
        return false;
    }

    esp_err_t err = nvs_set_str(nvs_handle_, key.data(), value.c_str());
    if (err != ESP_OK)
    {
        logger_.Error("Failed to set string for key: %s", key.data());
        return false;
    }
    logger_.Debug("string set for key: %s", key.data());
    return true;
}

bool Nvs::GetString(std::string_view key, std::string& out_value)
{
    if (nvs_handle_ == 0)
    {
        logger_.Error("namespace not opened");
        return false;
    }

    // First, get the required buffer size
    size_t required_size = 0;
    esp_err_t err = nvs_get_str(nvs_handle_, key.data(), nullptr, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        logger_.Debug("key not found: %s", key.data());
        return false;
    }
    if (err != ESP_OK)
    {
        logger_.Error("Failed to get string size for key: %s", key.data());
        return false;
    }

    if (required_size > 0)
    {
        // Allocate buffer and retrieve the string
        out_value.resize(required_size - 1);  // -1 to exclude null terminator
        err = nvs_get_str(nvs_handle_, key.data(), out_value.data(), &required_size);
        if (err != ESP_OK)
        {
            logger_.Error("Failed to get string for key: %s", key.data());
            return false;
        }
    }
    else
    {
        out_value.clear();
    }
    logger_.Debug("string retrieved for key: %s", key.data());
    return true;
}

// Explicit template instantiations for Nvs
template bool Nvs::SetValue<uint8_t>(std::string_view, uint8_t);
template bool Nvs::SetValue<int8_t>(std::string_view, int8_t);
template bool Nvs::SetValue<uint16_t>(std::string_view, uint16_t);
template bool Nvs::SetValue<int16_t>(std::string_view, int16_t);
template bool Nvs::SetValue<uint32_t>(std::string_view, uint32_t);
template bool Nvs::SetValue<int32_t>(std::string_view, int32_t);
template bool Nvs::SetValue<uint64_t>(std::string_view, uint64_t);
template bool Nvs::SetValue<int64_t>(std::string_view, int64_t);

template bool Nvs::GetValue<uint8_t>(std::string_view, uint8_t&);
template bool Nvs::GetValue<int8_t>(std::string_view, int8_t&);
template bool Nvs::GetValue<uint16_t>(std::string_view, uint16_t&);
template bool Nvs::GetValue<int16_t>(std::string_view, int16_t&);
template bool Nvs::GetValue<uint32_t>(std::string_view, uint32_t&);
template bool Nvs::GetValue<int32_t>(std::string_view, int32_t&);
template bool Nvs::GetValue<uint64_t>(std::string_view, uint64_t&);
template bool Nvs::GetValue<int64_t>(std::string_view, int64_t&);

// EventLoop Implementation

EventLoop::EventLoop(Logger& logger) : logger_(logger) {}

EventLoop::~EventLoop()
{
    if (loop_handle_)
    {
        esp_event_loop_delete(loop_handle_);
    }
}

bool EventLoop::CreateLoopDefault()
{
    if (loop_handle_)
    {
        logger_.Error("Cannot create default loop: Custom loop already exists in this object");
        return false;
    }

    esp_err_t err = esp_event_loop_create_default();
    if (err == ESP_OK)
    {
        is_default_loop_ = true;
        logger_.Info("Default event loop created");
    }
    else if (err == ESP_ERR_INVALID_STATE)
    {
        is_default_loop_ = true;
        logger_.Info("Default event loop already exists");
        return true;
    }
    else
    {
        logger_.Error("Failed to create default event loop");
    }
    return err == ESP_OK;
}

bool EventLoop::DeleteLoopDefault()
{
    esp_err_t err = esp_event_loop_delete_default();
    if (err == ESP_OK)
    {
        is_default_loop_ = false;
        logger_.Info("Default event loop deleted");
    }
    else
    {
        logger_.Error("Failed to delete default event loop");
    }
    return err == ESP_OK;
}

bool EventLoop::CreateLoop(const esp_event_loop_args_t& args)
{
    if (is_default_loop_)
    {
        logger_.Error("Cannot create custom loop: Default loop already managed by this object");
        return false;
    }
    if (loop_handle_)
    {
        logger_.Error("Custom loop already exists");
        return false;
    }

    esp_err_t err = esp_event_loop_create(&args, &loop_handle_);
    if (err == ESP_OK)
    {
        logger_.Info("Custom event loop created");
    }
    else
    {
        logger_.Error("Failed to create custom event loop");
    }
    return err == ESP_OK;
}

bool EventLoop::DeleteLoop()
{
    if (!loop_handle_)
        return false;

    esp_err_t err = esp_event_loop_delete(loop_handle_);
    if (err == ESP_OK)
    {
        loop_handle_ = nullptr;
        logger_.Info("Custom event loop deleted");
    }
    else
    {
        logger_.Error("Failed to delete custom event loop");
    }
    return err == ESP_OK;
}

bool EventLoop::RunLoop(TickType_t ticks_to_run)
{
    if (!loop_handle_)
    {
        logger_.Error("No custom loop to run");
        return false;
    }
    return esp_event_loop_run(loop_handle_, ticks_to_run) == ESP_OK;
}

bool EventLoop::Register(esp_event_base_t event_base,
                         int32_t event_id,
                         esp_event_handler_t event_handler,
                         void* event_handler_arg,
                         esp_event_handler_instance_t* instance)
{
    if (loop_handle_)
    {
        return esp_event_handler_instance_register_with(loop_handle_, event_base, event_id,
                                                        event_handler, event_handler_arg,
                                                        instance) == ESP_OK;
    }
    else if (is_default_loop_)
    {
        return esp_event_handler_instance_register(event_base, event_id, event_handler,
                                                   event_handler_arg, instance) == ESP_OK;
    }
    logger_.Error("No event loop initialized for Register");
    return false;
}

bool EventLoop::Unregister(esp_event_base_t event_base,
                           int32_t event_id,
                           esp_event_handler_instance_t instance)
{
    if (loop_handle_)
    {
        return esp_event_handler_instance_unregister_with(loop_handle_, event_base, event_id,
                                                          instance) == ESP_OK;
    }
    else if (is_default_loop_)
    {
        return esp_event_handler_instance_unregister(event_base, event_id, instance) == ESP_OK;
    }
    logger_.Error("No event loop initialized for Unregister");
    return false;
}

bool EventLoop::Post(esp_event_base_t event_base,
                     int32_t event_id,
                     const void* event_data,
                     size_t event_data_size,
                     TickType_t ticks_to_wait)
{
    if (loop_handle_)
    {
        return esp_event_post_to(loop_handle_, event_base, event_id, event_data, event_data_size,
                                 ticks_to_wait) == ESP_OK;
    }
    else if (is_default_loop_)
    {
        return esp_event_post(event_base, event_id, event_data, event_data_size, ticks_to_wait) ==
               ESP_OK;
    }
    logger_.Error("No event loop initialized for Post");
    return false;
}

bool EventLoop::PostFromIsr(esp_event_base_t event_base,
                            int32_t event_id,
                            const void* event_data,
                            size_t event_data_size,
                            BaseType_t* task_unblocked)
{
#if CONFIG_ESP_EVENT_POST_FROM_ISR
    if (loop_handle_)
    {
        return esp_event_isr_post_to(loop_handle_, event_base, event_id, event_data,
                                     event_data_size, task_unblocked) == ESP_OK;
    }
    else if (is_default_loop_)
    {
        return esp_event_isr_post(event_base, event_id, event_data, event_data_size,
                                  task_unblocked) == ESP_OK;
    }
    return false;
#else
    return false;
#endif
}

bool EventLoop::Dump(FILE* file) { return esp_event_dump(file) == ESP_OK; }
