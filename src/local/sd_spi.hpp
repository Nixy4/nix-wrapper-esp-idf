#pragma once

#include <optional>
#include <string>
#include <string_view>
#include "driver/gpio.h"
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "local/logger.hpp"
#include "local/sd_types.hpp"
#include "local/spi.hpp"

namespace wrapper
{

/**
 * @brief FAT 挂载配置（SD SPI 模式）
 */
struct SdSpiMountConfig : public esp_vfs_fat_mount_config_t
{
    SdSpiMountConfig(bool   format_if_mount_failed   = false,
                     int    max_files                = 5,
                     size_t allocation_unit_size     = 0,
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
 * @brief SPI 设备配置（CS / CD / WP 引脚）
 *
 * host_id 无需在构造时指定，SdSpi::Init() 会从传入的 SpiBus 中自动读取并覆盖。
 */
struct SdSpiDevConfig : public sdspi_device_config_t
{
    SdSpiDevConfig(gpio_num_t gpio_cs,
                   gpio_num_t gpio_cd = SDSPI_SLOT_NO_CD,
                   gpio_num_t gpio_wp = SDSPI_SLOT_NO_WP)
        : sdspi_device_config_t{}
    {
        this->host_id          = SDSPI_DEFAULT_HOST;  // 由 Init() 覆盖
        this->gpio_cs          = gpio_cs;
        this->gpio_cd          = gpio_cd;
        this->gpio_wp          = gpio_wp;
        this->gpio_int         = SDSPI_SLOT_NO_INT;
        this->gpio_wp_polarity = SDSPI_IO_ACTIVE_LOW;
        this->duty_cycle_pos   = 0;
        this->wait_for_miso    = 0;
    }
};

/**
 * @brief SPI 模式 SD 卡 wrapper（FAT 文件系统，通过 VFS 挂载）
 *
 * 使用前提：SpiBus 必须已完成初始化（SpiBus::Init()）。
 *
 * 调用流程：
 *   1. SpiBus::Init(...)
 *   2. SdSpi::Init(spi_bus, dev_config, mount_config, "/sdcard")
 *   3. 使用 POSIX / C 文件 API 访问 "/sdcard/..."
 *   4. SdSpi::Deinit()
 */
class SdSpi
{
    Logger&       logger_;
    sdmmc_card_t* card_;
    bool          mounted_;
    std::string   base_path_;

   public:
    explicit SdSpi(Logger& logger);
    ~SdSpi();

    SdSpi(const SdSpi&)            = delete;
    SdSpi& operator=(const SdSpi&) = delete;

    // -----------------------------------------------------------------------
    // 生命周期
    // -----------------------------------------------------------------------

    /**
     * @brief 挂载 SD 卡（SPI 模式）到 VFS FAT 文件系统
     *
     * @param spi_bus      已初始化的 SPI 总线（用于获取 host_id）
     * @param dev_config   SPI 设备配置（CS 引脚等）
     * @param mount_config FAT 挂载配置
     * @param base_path    VFS 挂载路径，默认 "/sdcard"
     */
    bool Init(const SpiBus&           spi_bus,
              const SdSpiDevConfig&   dev_config,
              const SdSpiMountConfig& mount_config,
              std::string_view        base_path = "/sdcard");

    /** @brief 卸载 SD 卡，释放 VFS 资源 */
    bool Deinit();

    // -----------------------------------------------------------------------
    // 状态查询
    // -----------------------------------------------------------------------

    bool               IsMounted()   const { return mounted_; }
    const std::string& GetBasePath() const { return base_path_; }

    /**
     * @brief 查询 SD 卡属性（名称、容量、频率等）
     * @return 成功时返回 SdCardInfo，失败或未挂载时返回 std::nullopt
     */
    std::optional<SdCardInfo> GetCardInfo() const;

    /**
     * @brief 检查 SD 卡在线状态（发送 CMD13 获取状态寄存器）
     * @return true = 卡响应正常
     */
    bool GetStatus() const;

    /**
     * @brief 查询卡是否支持 Discard 操作
     * @return true = 支持
     */
    bool CanDiscard() const;

    /**
     * @brief 查询卡是否支持 Trim 操作
     * @return true = 支持
     */
    bool CanTrim() const;

    // -----------------------------------------------------------------------
    // 扇区级 I/O
    // -----------------------------------------------------------------------

    /**
     * @brief 读取扇区数据
     *
     * @param start_sector  起始扇区编号
     * @param sector_count  扇区数量
     * @param dst           目标缓冲区，大小 >= sector_count * sector_size（需 DMA 对齐）
     */
    bool ReadSectors(size_t start_sector, size_t sector_count, void* dst) const;

    /**
     * @brief 写入扇区数据
     *
     * @param start_sector  起始扇区编号
     * @param sector_count  扇区数量
     * @param src           源缓冲区，大小 >= sector_count * sector_size（需 DMA 对齐）
     */
    bool WriteSectors(size_t start_sector, size_t sector_count, const void* src) const;

    /**
     * @brief 擦除扇区
     *
     * @param start_sector  起始扇区编号
     * @param sector_count  扇区数量
     * @param mode          擦除模式（Erase / Discard），默认 Erase
     * @note  SPI 模式下 Erase 后建议重新初始化卡
     */
    bool EraseSectors(size_t       start_sector,
                      size_t       sector_count,
                      SdEraseMode  mode = SdEraseMode::Erase) const;

    // -----------------------------------------------------------------------
    // 文件系统操作
    // -----------------------------------------------------------------------

    /**
     * @brief 格式化 SD 卡上的 FAT 文件系统（卡需处于已挂载状态）
     */
    bool Format();

    /**
     * @brief 向标准输出打印卡信息（调用 sdmmc_card_print_info）
     */
    void PrintInfo() const;
};

}  // namespace wrapper
