#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include "esp_spiffs.h"
#include "wrapper/logger.hpp"

namespace wrapper
{

/**
 * @brief SPIFFS 文件系统容量信息
 */
struct SpiffsInfo
{
    size_t total_bytes;  ///< 分区总字节数
    size_t used_bytes;   ///< 已使用字节数
};

/**
 * @brief SPIFFS 初始化配置（继承 IDF esp_vfs_spiffs_conf_t）
 *
 * @note  base_path 和 partition_label 须为静态 C 字符串（IDF 要求生命周期覆盖挂载期间），
 *        通常直接传字符串字面量。
 */
struct SpiffsConfig : public esp_vfs_spiffs_conf_t
{
    SpiffsConfig(const char* base_path,
                 const char* partition_label = nullptr,
                 size_t max_files = 5,
                 bool format_if_mount_failed = false)
        : esp_vfs_spiffs_conf_t{}
    {
        this->base_path = base_path;
        this->partition_label = partition_label;
        this->max_files = max_files;
        this->format_if_mount_failed = format_if_mount_failed;
    }
};

/**
 * @brief SPIFFS 文件系统 wrapper（通过 VFS 挂载到 ESP-IDF 虚拟文件系统）
 *
 * 调用流程：
 *   1. SpiffsConfig cfg("/spiffs", nullptr, 5, true);
 *   2. Spiffs::Init(cfg)
 *   3. 使用 POSIX / C 文件 API 访问 "/spiffs/..."
 *   4. Spiffs::Deinit()
 */
class Spiffs
{
    Logger& logger_;
    bool mounted_;
    std::string partition_label_;  ///< 空字符串表示使用默认分区
    std::string base_path_;        ///< VFS 挂载点

   public:
    explicit Spiffs(Logger& logger);
    ~Spiffs();

    Spiffs(const Spiffs&) = delete;
    Spiffs& operator=(const Spiffs&) = delete;

    // -----------------------------------------------------------------------
    // 生命周期
    // -----------------------------------------------------------------------

    /** @brief 注册并挂载 SPIFFS 文件系统 */
    bool Init(const SpiffsConfig& config);

    /** @brief 注销 SPIFFS 文件系统，释放 VFS 资源 */
    bool Deinit();

    // -----------------------------------------------------------------------
    // 状态查询
    // -----------------------------------------------------------------------

    bool IsMounted() const { return mounted_; }
    const std::string& GetBasePath() const { return base_path_; }
    const std::string& GetPartitionLabel() const { return partition_label_; }

    /**
     * @brief 获取 SPIFFS 容量信息
     * @return 成功时返回 SpiffsInfo，失败或未挂载时返回 std::nullopt
     */
    std::optional<SpiffsInfo> GetInfo() const;

    // -----------------------------------------------------------------------
    // 文件系统操作
    // -----------------------------------------------------------------------

    /**
     * @brief 格式化 SPIFFS 分区（文件系统需处于已挂载状态）
     */
    bool Format();

    /**
     * @brief 一致性检查（遍历并修复目录项损坏等问题）
     */
    bool Check();

    /**
     * @brief 手动触发垃圾回收，释放已删除文件占用的空间
     *
     * @param size_to_gc  目标回收字节数；传入 0 则由 SPIFFS 自行决定回收量
     */
    bool GarbageCollect(size_t size_to_gc);

    // -----------------------------------------------------------------------
    // 测试
    // -----------------------------------------------------------------------

    /**
     * @brief 对已挂载的 SPIFFS 执行读写验证（需先调用 Init()）
     * @return true = 全部步骤成功
     */
    bool Test();

   private:
    /** @brief 返回用于 IDF API 的分区标签（空字符串 → nullptr）*/
    const char* Label() const
    {
        return partition_label_.empty() ? nullptr : partition_label_.c_str();
    }
};

}  // namespace wrapper
