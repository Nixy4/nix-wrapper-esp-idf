#pragma once

#include <optional>
#include <string>
#include <string_view>
#include "driver/gpio.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "wrapper/logger.hpp"
#include "wrapper/sd_types.hpp"

namespace wrapper
{

/**
 * @brief FAT 挂载配置（SD SDMMC 模式）
 */
struct SdMmcMountConfig : public esp_vfs_fat_mount_config_t
{
    SdMmcMountConfig(bool format_if_mount_failed = false,
                     int max_files = 5,
                     size_t allocation_unit_size = 0,
                     bool disk_status_check_enable = false)
        : esp_vfs_fat_mount_config_t{}
    {
        this->format_if_mount_failed = format_if_mount_failed;
        this->max_files = max_files;
        this->allocation_unit_size = allocation_unit_size;
        this->disk_status_check_enable = disk_status_check_enable;
        this->use_one_fat = false;
    }
};

/**
 * @brief SDMMC 槽位配置（时钟/命令/数据引脚）
 */
struct SdMmcSlotConfig : public sdmmc_slot_config_t
{
    SdMmcSlotConfig(gpio_num_t gpio_clk,
                    gpio_num_t gpio_cmd,
                    gpio_num_t gpio_d0,
                    gpio_num_t gpio_d1 = GPIO_NUM_NC,
                    gpio_num_t gpio_d2 = GPIO_NUM_NC,
                    gpio_num_t gpio_d3 = GPIO_NUM_NC,
                    gpio_num_t gpio_d4 = GPIO_NUM_NC,
                    gpio_num_t gpio_d5 = GPIO_NUM_NC,
                    gpio_num_t gpio_d6 = GPIO_NUM_NC,
                    gpio_num_t gpio_d7 = GPIO_NUM_NC,
                    uint8_t width = SDMMC_SLOT_WIDTH_DEFAULT,
                    gpio_num_t gpio_cd = SDMMC_SLOT_NO_CD,
                    gpio_num_t gpio_wp = SDMMC_SLOT_NO_WP,
                    uint32_t flags = 0)
        : sdmmc_slot_config_t{}
    {
        this->clk = gpio_clk;
        this->cmd = gpio_cmd;
        this->d0 = gpio_d0;
        this->d1 = gpio_d1;
        this->d2 = gpio_d2;
        this->d3 = gpio_d3;
        this->d4 = gpio_d4;
        this->d5 = gpio_d5;
        this->d6 = gpio_d6;
        this->d7 = gpio_d7;
        this->cd = gpio_cd;
        this->wp = gpio_wp;
        this->width = width;
        this->flags = flags;
    }
};

/**
 * @brief SDMMC（原生 MMC 总线）模式 SD 卡 wrapper（FAT 文件系统，通过 VFS 挂载）
 *
 * 调用流程：
 *   1. SdMmc::Init(slot, slot_config, mount_config, "/sdcard")
 *   2. 使用 POSIX / C 文件 API 访问 "/sdcard/..."
 *   3. SdMmc::Deinit()
 */
class SdMmc
{
    Logger& logger_;
    sdmmc_card_t* card_;
    int slot_;
    bool mounted_;
    std::string base_path_;

   public:
    explicit SdMmc(Logger& logger);
    ~SdMmc();

    SdMmc(const SdMmc&) = delete;
    SdMmc& operator=(const SdMmc&) = delete;

    // -----------------------------------------------------------------------
    // 生命周期
    // -----------------------------------------------------------------------

    /**
     * @brief 挂载 SD 卡（SDMMC 模式）到 VFS FAT 文件系统
     *
     * @param slot         SDMMC 槽位号（通常 0 或 1）
     * @param slot_config  槽位引脚配置
     * @param mount_config FAT 挂载配置
     * @param base_path    VFS 挂载路径，默认 "/sdcard"
     */
    bool Init(int slot,
              const SdMmcSlotConfig& slot_config,
              const SdMmcMountConfig& mount_config,
              std::string_view base_path = "/sdcard");

    /** @brief 卸载 SD 卡，释放 VFS 资源 */
    bool Deinit();

    // -----------------------------------------------------------------------
    // 状态查询
    // -----------------------------------------------------------------------

    bool IsMounted() const { return mounted_; }
    const std::string& GetBasePath() const { return base_path_; }

    /**
     * @brief 查询 SD 卡属性（名称、容量、频率等）
     * @return 成功时返回 SdCardInfo，失败或未挂载时返回 std::nullopt
     */
    std::optional<SdCardInfo> GetCardInfo() const;

    /**
     * @brief 检查 SD 卡在线状态（发送 CMD13）
     * @return true = 卡响应正常
     */
    bool GetStatus() const;

    /**
     * @brief 查询卡是否支持 Discard
     */
    bool CanDiscard() const;

    /**
     * @brief 查询卡是否支持 Trim
     */
    bool CanTrim() const;

    // -----------------------------------------------------------------------
    // 总线运行时控制（需在 Init 之后调用）
    // -----------------------------------------------------------------------

    /**
     * @brief 动态设置总线位宽（1 / 4 / 8）
     * @param width  目标位宽
     */
    bool SetBusWidth(uint8_t width);

    /**
     * @brief 动态调整时钟频率
     * @param freq_khz  目标频率（kHz），例如 20000 = 20 MHz
     */
    bool SetClockFreq(uint32_t freq_khz);

    // -----------------------------------------------------------------------
    // 扇区级 I/O
    // -----------------------------------------------------------------------

    /** @brief 读取扇区（dst 需 DMA 对齐）*/
    bool ReadSectors(size_t start_sector, size_t sector_count, void* dst) const;

    /** @brief 写入扇区（src 需 DMA 对齐）*/
    bool WriteSectors(size_t start_sector, size_t sector_count, const void* src) const;

    /**
     * @brief 擦除扇区
     * @param mode  擦除模式（Erase / Discard），默认 Erase
     */
    bool EraseSectors(size_t start_sector,
                      size_t sector_count,
                      SdEraseMode mode = SdEraseMode::Erase) const;

    // -----------------------------------------------------------------------
    // 文件系统操作
    // -----------------------------------------------------------------------

    /** @brief 格式化 SD 卡上的 FAT 文件系统（卡需处于已挂载状态）*/
    bool Format();

    /** @brief 向标准输出打印卡信息 */
    void PrintInfo() const;

    // -----------------------------------------------------------------------
    // 测试
    // -----------------------------------------------------------------------

    /**
     * @brief 对已挂载的 SD 卡执行读写验证（需先调用 Init()）
     * @return true = 全部步骤成功
     */
    bool Test();
};

}  // namespace wrapper
