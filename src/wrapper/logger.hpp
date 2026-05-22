#pragma once

#include <string>
#include <vector>
#include <initializer_list>
#include "esp_log.h"

namespace wrapper
{

class Logger
{
   private:
    std::string prefix_{"| "};
    std::string separator_{" | "};
    std::string suffix_{" |"};
    std::string tags_{"None"};
    std::vector<std::string> tags_list_;

    void RebuildTags();

   public:
    Logger();
    Logger(const std::string& tags);  // Add this constructor
    template <typename... Tags>
    Logger(const Tags&... tags) : Logger()
    {
        SetTags(tags...);
    }
    ~Logger();

    void SetPrefix(const std::string& prefix);
    void SetSeparator(const std::string& separator);
    void SetSuffix(const std::string& suffix);

    // Variadic template for multiple string tags
    template <typename... Tags>
    void SetTags(Tags&&... tags)
    {
        tags_list_.clear();
        tags_list_.reserve(sizeof...(tags));
        (tags_list_.push_back(std::forward<Tags>(tags)), ...);
        RebuildTags();
    }

    void SetTags(const std::vector<std::string>& tags);
    void SetTags(const std::initializer_list<std::string>& tags);
    void SetTag(int index, const std::string& tag);
    void ClearTags();

    template <typename... Args>
    void Verbose(const char* fmt, Args&&... args)
    {
        ESP_LOGV(tags_.c_str(), fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void Debug(const char* fmt, Args&&... args)
    {
        ESP_LOGD(tags_.c_str(), fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void Info(const char* fmt, Args&&... args)
    {
        ESP_LOGI(tags_.c_str(), fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void Warning(const char* fmt, Args&&... args)
    {
        ESP_LOGW(tags_.c_str(), fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void Error(const char* fmt, Args&&... args)
    {
        ESP_LOGE(tags_.c_str(), fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void Fatal(const char* fmt, Args&&... args)
    {
        ESP_LOGE(tags_.c_str(), fmt, std::forward<Args>(args)...);
    }
};

}  // namespace wrapper