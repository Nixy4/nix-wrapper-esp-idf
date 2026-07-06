#include "local/spiffs.hpp"

namespace wrapper
{

Spiffs::Spiffs(Logger& logger) : logger_(logger), mounted_(false) {}

Spiffs::~Spiffs() { Deinit(); }

// ---------------------------------------------------------------------------
// 生命周期
// ---------------------------------------------------------------------------

bool Spiffs::Init(const SpiffsConfig& config)
{
    if (mounted_)
    {
        logger_.Warning("Already mounted. Deinitializing first.");
        Deinit();
    }

    // 保存分区标签和挂载点（nullptr 表示使用默认分区）
    partition_label_ = config.partition_label ? config.partition_label : "";
    base_path_ = config.base_path ? config.base_path : "";

    esp_err_t ret = esp_vfs_spiffs_register(&config);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to mount SPIFFS (label=%s): %s",
                      config.partition_label ? config.partition_label : "(default)",
                      esp_err_to_name(ret));
        partition_label_.clear();
        base_path_.clear();
        mounted_ = false;
        return false;
    }

    mounted_ = true;
    logger_.Info("Mounted SPIFFS at %s (label=%s)", config.base_path,
                 config.partition_label ? config.partition_label : "(default)");
    return true;
}

bool Spiffs::Deinit()
{
    if (!mounted_)
    {
        return true;
    }

    esp_err_t ret = esp_vfs_spiffs_unregister(Label());
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to unmount SPIFFS: %s", esp_err_to_name(ret));
        return false;
    }

    mounted_ = false;
    logger_.Info("Unmounted SPIFFS (label=%s)",
                 partition_label_.empty() ? "(default)" : partition_label_.c_str());
    partition_label_.clear();
    base_path_.clear();
    return true;
}

// ---------------------------------------------------------------------------
// 状态查询
// ---------------------------------------------------------------------------

std::optional<SpiffsInfo> Spiffs::GetInfo() const
{
    if (!mounted_)
    {
        logger_.Warning("Not mounted, cannot get SPIFFS info");
        return std::nullopt;
    }

    size_t total = 0;
    size_t used = 0;
    esp_err_t ret = esp_spiffs_info(Label(), &total, &used);
    if (ret != ESP_OK)
    {
        logger_.Error("esp_spiffs_info failed: %s", esp_err_to_name(ret));
        return std::nullopt;
    }
    return SpiffsInfo{total, used};
}

// ---------------------------------------------------------------------------
// 文件系统操作
// ---------------------------------------------------------------------------

bool Spiffs::Format()
{
    if (!mounted_)
    {
        logger_.Error("Not mounted, cannot format SPIFFS");
        return false;
    }
    esp_err_t ret = esp_spiffs_format(Label());
    if (ret != ESP_OK)
    {
        logger_.Error("SPIFFS format failed: %s", esp_err_to_name(ret));
        return false;
    }
    logger_.Info("Formatted SPIFFS (label=%s)",
                 partition_label_.empty() ? "(default)" : partition_label_.c_str());
    return true;
}

bool Spiffs::Check()
{
    if (!mounted_)
    {
        logger_.Error("Not mounted, cannot check SPIFFS");
        return false;
    }
    esp_err_t ret = esp_spiffs_check(Label());
    if (ret != ESP_OK)
    {
        logger_.Error("SPIFFS check failed: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool Spiffs::GarbageCollect(size_t size_to_gc)
{
    if (!mounted_)
    {
        logger_.Error("Not mounted, cannot run GC");
        return false;
    }
    esp_err_t ret = esp_spiffs_gc(Label(), size_to_gc);
    if (ret != ESP_OK)
    {
        logger_.Error("SPIFFS GC failed (size=%zu): %s", size_to_gc, esp_err_to_name(ret));
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// 测试
// ---------------------------------------------------------------------------

bool Spiffs::Test()
{
    if (!mounted_)
    {
        logger_.Error("Test: not mounted, call Init() first");
        return false;
    }

    auto info = GetInfo();
    if (info)
        logger_.Info("Total: %zu B, Used: %zu B", info->total_bytes, info->used_bytes);

    const std::string test_path = base_path_ + "/test.txt";
    bool ok = true;

    // Write
    FILE* f = fopen(test_path.c_str(), "w");
    if (f)
    {
        fprintf(f, "Hello from SPIFFS!");
        fclose(f);
        logger_.Info("Write OK");
    }
    else
    {
        logger_.Error("Open for write failed");
        ok = false;
    }

    // Read back
    f = fopen(test_path.c_str(), "r");
    if (f)
    {
        char buf[64] = {};
        fread(buf, 1, sizeof(buf) - 1, f);
        fclose(f);
        logger_.Info("Read back: \"%s\"", buf);
    }
    else
    {
        logger_.Error("Open for read failed");
        ok = false;
    }

    return ok;
}

}  // namespace wrapper
