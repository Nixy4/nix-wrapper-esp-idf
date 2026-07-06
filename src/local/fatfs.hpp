#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include "esp_vfs_fat.h"
#include "wear_levelling.h"
#include "local/logger.hpp"

namespace wrapper
{

/**
 * @brief FAT 文件系统容量信息
 */
struct FsInfo
{
    uint64_t total_bytes; ///< 分区总字节数
    uint64_t free_bytes;  ///< 可用字节数
};

/**
 * @brief Flash FAT 文件系统挂载配置
 */
struct FatFsMountConfig : public esp_vfs_fat_mount_config_t
{
    FatFsMountConfig(bool   format_if_mount_failed   = false,
                     int    max_files                = 5,
                     size_t allocation_unit_size     = 4096,
                     bool   disk_status_check_enable = false)
        : esp_vfs_fat_mount_config_t{}
    {
        this->format_if_mount_failed   = format_if_mount_failed;
        this->max_files                = max_files;
        this->allocation_unit_size     = allocation_unit_size;
        this->disk_status_check_enable = disk_status_check_enable;
        this->use_one_fat              = false;
    }
};

/**
 * @brief 基于磨损均衡（Wear-Levelling）的 Flash FAT 文件系统 wrapper
 *
 * 调用流程：
 *   1. FatFs::Init("storage", mount_config, "/ffat")
 *   2. 使用 POSIX / C 文件 API 访问 "/ffat/..."
 *   3. FatFs::Deinit()
 */
class FatFs
{
    Logger&     logger_;
    wl_handle_t wl_handle_;
    bool        mounted_;
    std::string base_path_;
    std::string partition_label_;

   public:
    explicit FatFs(Logger& logger);
    ~FatFs();

    FatFs(const FatFs&)            = delete;
    FatFs& operator=(const FatFs&) = delete;

    // -----------------------------------------------------------------------
    // 生命周期
    // -----------------------------------------------------------------------

    /**
     * @brief 挂载 Flash FAT 文件系统到 VFS
     *
     * @param partition_label  NVS 分区标签（需在分区表中存在）
     * @param mount_config     FAT 挂载配置
     * @param base_path        VFS 挂载路径，默认 "/ffat"
     */
    bool Init(std::string_view        partition_label,
              const FatFsMountConfig& mount_config,
              std::string_view        base_path = "/ffat");

    /** @brief 卸载文件系统，释放磨损均衡句柄 */
    bool Deinit();

    // -----------------------------------------------------------------------
    // 状态查询
    // -----------------------------------------------------------------------

    bool               IsMounted()   const { return mounted_; }
    const std::string& GetBasePath() const { return base_path_; }
    const std::string& GetLabel()    const { return partition_label_; }
    wl_handle_t        GetWlHandle() const { return wl_handle_; }

    /**
     * @brief 获取文件系统容量信息
     * @return 成功时返回 FsInfo，失败或未挂载时返回 std::nullopt
     */
    std::optional<FsInfo> GetInfo() const;

    // -----------------------------------------------------------------------
    // 文件系统操作
    // -----------------------------------------------------------------------

    /**
     * @brief 格式化并重新挂载（文件系统需处于已挂载状态）
     */
    bool Format();

    /**
     * @brief 在 FAT 分区上创建连续文件（对需要 DMA 连续访问的场景有用）
     *
     * @param full_path   完整文件路径，例如 "/ffat/data.bin"
     * @param size        文件大小（字节）
     * @param alloc_now   true = 立即分配扇区；false = 仅预留 FAT 链但不写零
     */
    bool CreateContiguousFile(std::string_view full_path,
                              uint64_t         size,
                              bool             alloc_now = true);

    /**
     * @brief 测试指定文件是否为连续存储
     *
     * @param full_path  完整文件路径
     * @return true = 连续，false = 不连续，std::nullopt = 查询失败
     */
    std::optional<bool> IsContiguousFile(std::string_view full_path) const;
};

}  // namespace wrapper
