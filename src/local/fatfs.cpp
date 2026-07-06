#include "local/fatfs.hpp"

namespace wrapper
{

FatFs::FatFs(Logger& logger)
    : logger_(logger), wl_handle_(WL_INVALID_HANDLE), mounted_(false)
{
}

FatFs::~FatFs() { Deinit(); }

// ---------------------------------------------------------------------------
// 生命周期
// ---------------------------------------------------------------------------

bool FatFs::Init(std::string_view        partition_label,
                 const FatFsMountConfig& mount_config,
                 std::string_view        base_path)
{
    if (mounted_)
    {
        logger_.Warning("Already mounted. Deinitializing first.");
        Deinit();
    }

    base_path_       = std::string(base_path);
    partition_label_ = std::string(partition_label);

    esp_err_t ret = esp_vfs_fat_spiflash_mount_rw_wl(
        base_path_.c_str(),
        partition_label_.c_str(),
        &mount_config,
        &wl_handle_);

    if (ret != ESP_OK)
    {
        logger_.Error("Failed to mount FAT FS (label=%s): %s",
                      partition_label_.c_str(), esp_err_to_name(ret));
        wl_handle_ = WL_INVALID_HANDLE;
        mounted_   = false;
        base_path_.clear();
        partition_label_.clear();
        return false;
    }

    mounted_ = true;
    logger_.Info("Mounted FAT FS at %s (label=%s)", base_path_.c_str(),
                 partition_label_.c_str());
    return true;
}

bool FatFs::Deinit()
{
    if (!mounted_)
    {
        return true;
    }

    esp_err_t ret = esp_vfs_fat_spiflash_unmount_rw_wl(base_path_.c_str(), wl_handle_);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to unmount FAT FS: %s", esp_err_to_name(ret));
        return false;
    }

    wl_handle_ = WL_INVALID_HANDLE;
    mounted_   = false;
    logger_.Info("Unmounted FAT FS from %s", base_path_.c_str());
    base_path_.clear();
    partition_label_.clear();
    return true;
}

// ---------------------------------------------------------------------------
// 状态查询
// ---------------------------------------------------------------------------

std::optional<FsInfo> FatFs::GetInfo() const
{
    if (!mounted_)
    {
        logger_.Warning("Not mounted, cannot get FAT FS info");
        return std::nullopt;
    }

    uint64_t total = 0;
    uint64_t free  = 0;
    esp_err_t ret  = esp_vfs_fat_info(base_path_.c_str(), &total, &free);
    if (ret != ESP_OK)
    {
        logger_.Error("esp_vfs_fat_info failed: %s", esp_err_to_name(ret));
        return std::nullopt;
    }
    return FsInfo{total, free};
}

// ---------------------------------------------------------------------------
// 文件系统操作
// ---------------------------------------------------------------------------

bool FatFs::Format()
{
    if (!mounted_)
    {
        logger_.Error("Not mounted, cannot format");
        return false;
    }

    esp_err_t ret = esp_vfs_fat_spiflash_format_rw_wl(base_path_.c_str(),
                                                       partition_label_.c_str());
    if (ret != ESP_OK)
    {
        logger_.Error("Format failed: %s", esp_err_to_name(ret));
        return false;
    }
    logger_.Info("Formatted FAT FS at %s", base_path_.c_str());
    return true;
}

bool FatFs::CreateContiguousFile(std::string_view full_path,
                                 uint64_t         size,
                                 bool             alloc_now)
{
    if (!mounted_)
    {
        logger_.Error("Not mounted, cannot create contiguous file");
        return false;
    }

    esp_err_t ret = esp_vfs_fat_create_contiguous_file(
        base_path_.c_str(),
        std::string(full_path).c_str(),
        size,
        alloc_now);

    if (ret != ESP_OK)
    {
        logger_.Error("CreateContiguousFile(%.*s) failed: %s",
                      static_cast<int>(full_path.size()), full_path.data(),
                      esp_err_to_name(ret));
        return false;
    }
    return true;
}

std::optional<bool> FatFs::IsContiguousFile(std::string_view full_path) const
{
    if (!mounted_)
    {
        logger_.Warning("Not mounted, cannot test contiguous file");
        return std::nullopt;
    }

    bool      is_contiguous = false;
    esp_err_t ret = esp_vfs_fat_test_contiguous_file(
        base_path_.c_str(),
        std::string(full_path).c_str(),
        &is_contiguous);

    if (ret != ESP_OK)
    {
        logger_.Error("IsContiguousFile(%.*s) failed: %s",
                      static_cast<int>(full_path.size()), full_path.data(),
                      esp_err_to_name(ret));
        return std::nullopt;
    }
    return is_contiguous;
}

}  // namespace wrapper
