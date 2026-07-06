#pragma once
#include <nvs_flash.h>
#include <esp_event.h>
#include <esp_wifi.h>
#include <esp_mac.h>
#include <string>
#include <string_view>
#include "local/logger.hpp"

namespace wrapper
{

class Nvs
{
    Logger& logger_;
    nvs_handle_t nvs_handle_;

   public:
    Nvs(Logger& logger);
    ~Nvs();
    bool Init();
    bool Erase();
    bool Commit();

    bool OpenNamespace(std::string_view namespace_name, nvs_open_mode_t open_mode);
    bool EraseKey(std::string_view key);

    template <typename T>
    bool SetValue(std::string_view key, T value);
    template <typename T>
    bool GetValue(std::string_view key, T& out_value);

    bool SetString(std::string_view key, std::string value);
    bool GetString(std::string_view key, std::string& out_value);
};

class EventLoop
{
    Logger& logger_;
    esp_event_loop_handle_t loop_handle_ = nullptr;
    bool is_default_loop_ = false;

   public:
    EventLoop(Logger& logger);
    ~EventLoop();

    // Loop Management
    bool CreateLoopDefault();
    bool DeleteLoopDefault();

    bool CreateLoop(const esp_event_loop_args_t& args);
    bool DeleteLoop();

    bool RunLoop(TickType_t ticks_to_run);

    // Handler Management
    bool Register(esp_event_base_t event_base,
                  int32_t event_id,
                  esp_event_handler_t event_handler,
                  void* event_handler_arg,
                  esp_event_handler_instance_t* instance);
    bool Unregister(esp_event_base_t event_base,
                    int32_t event_id,
                    esp_event_handler_instance_t instance);

    // Post Events
    bool Post(esp_event_base_t event_base,
              int32_t event_id,
              const void* event_data,
              size_t event_data_size,
              TickType_t ticks_to_wait);

    // ISR Post
    bool PostFromIsr(esp_event_base_t event_base,
                     int32_t event_id,
                     const void* event_data,
                     size_t event_data_size,
                     BaseType_t* task_unblocked);

    // Diagnostics
    bool Dump(FILE* file);
};

#include <esp_efuse.h>

struct SocMac
{
    uint8_t byte_[6];

    uint8_t GetByte(size_t index) const
    {
        if (index >= 6)
        {
            return 0;
        }
        return byte_[index];
    }

    std::string GetString() const
    {
        char buffer[18];
        snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X", byte_[0], byte_[1],
                 byte_[2], byte_[3], byte_[4], byte_[5]);
        return std::string(buffer);
    }
};

inline bool SocGetBaseMac(SocMac& base_mac)
{
    uint8_t mac[6];
    esp_err_t err = esp_base_mac_addr_get(mac);
    if (err != ESP_OK)
    {
        return false;
    }
    std::copy(mac, mac + 6, base_mac.byte_);
    return true;
}

}  // namespace wrapper
