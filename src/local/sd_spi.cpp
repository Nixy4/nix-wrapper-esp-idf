#include "local/sd_spi.hpp"
#include <cstdio>
#include <cstring>

namespace wrapper
{

SdSpi::SdSpi(Logger& logger) : logger_(logger), card_(nullptr), mounted_(false) {}

SdSpi::~SdSpi() { Deinit(); }

// ---------------------------------------------------------------------------
// 生命周期
// ---------------------------------------------------------------------------

bool SdSpi::Init(const SpiBus& spi_bus,
                 const SdSpiDevConfig& dev_config,
                 const SdSpiMountConfig& mount_config,
                 std::string_view base_path)
{
    if (mounted_)
    {
        logger_.Warning("Already mounted. Deinitializing first.");
        Deinit();
    }

    // 从 SpiBus 中读取 host_id 并注入设备配置
    sdspi_device_config_t cfg = dev_config;
    cfg.host_id = static_cast<spi_host_device_t>(spi_bus.GetHostId());

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = static_cast<int>(cfg.host_id);

    // 先存储路径，IDF 挂载时使用 c_str()
    base_path_ = std::string(base_path);

    esp_err_t ret = esp_vfs_fat_sdspi_mount(base_path_.c_str(), &host, &cfg, &mount_config, &card_);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to mount SD (SPI): %s", esp_err_to_name(ret));
        card_ = nullptr;
        mounted_ = false;
        base_path_.clear();
        return false;
    }

    mounted_ = true;
    logger_.Info("Mounted at %s (CS: %d)", base_path_.c_str(), cfg.gpio_cs);
    return true;
}

bool SdSpi::Deinit()
{
    if (!mounted_)
    {
        return true;
    }

    esp_err_t ret = esp_vfs_fat_sdcard_unmount(base_path_.c_str(), card_);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to unmount SD (SPI): %s", esp_err_to_name(ret));
        return false;
    }

    card_ = nullptr;
    mounted_ = false;
    logger_.Info("Unmounted from %s", base_path_.c_str());
    base_path_.clear();
    return true;
}

// ---------------------------------------------------------------------------
// 状态查询
// ---------------------------------------------------------------------------

std::optional<SdCardInfo> SdSpi::GetCardInfo() const
{
    if (!mounted_ || card_ == nullptr)
    {
        logger_.Warning("Card not mounted, cannot get info");
        return std::nullopt;
    }

    SdCardInfo info;
    info.name = std::string(card_->cid.name);
    info.sector_count = static_cast<uint32_t>(card_->csd.capacity);
    info.sector_size = static_cast<uint32_t>(card_->csd.sector_size);
    info.capacity_bytes = static_cast<uint64_t>(info.sector_count) * info.sector_size;
    info.max_freq_khz = card_->max_freq_khz;
    info.real_freq_khz = card_->real_freq_khz;
    info.is_mmc = static_cast<bool>(card_->is_mmc);
    info.is_sdio = static_cast<bool>(card_->is_sdio);
    return info;
}

bool SdSpi::GetStatus() const
{
    if (!mounted_ || card_ == nullptr)
    {
        logger_.Warning("Card not mounted, cannot get status");
        return false;
    }
    esp_err_t ret = sdmmc_get_status(card_);
    if (ret != ESP_OK)
    {
        logger_.Warning("Card status check failed: %s", esp_err_to_name(ret));
    }
    return ret == ESP_OK;
}

bool SdSpi::CanDiscard() const
{
    if (!mounted_ || card_ == nullptr)
        return false;
    return sdmmc_can_discard(card_) == ESP_OK;
}

bool SdSpi::CanTrim() const
{
    if (!mounted_ || card_ == nullptr)
        return false;
    return sdmmc_can_trim(card_) == ESP_OK;
}

// ---------------------------------------------------------------------------
// 扇区级 I/O
// ---------------------------------------------------------------------------

bool SdSpi::ReadSectors(size_t start_sector, size_t sector_count, void* dst) const
{
    if (!mounted_ || card_ == nullptr)
    {
        logger_.Error("Card not mounted, cannot read");
        return false;
    }
    esp_err_t ret = sdmmc_read_sectors(card_, dst, start_sector, sector_count);
    if (ret != ESP_OK)
    {
        logger_.Error("ReadSectors failed (start=%zu, count=%zu): %s", start_sector, sector_count,
                      esp_err_to_name(ret));
    }
    return ret == ESP_OK;
}

bool SdSpi::WriteSectors(size_t start_sector, size_t sector_count, const void* src) const
{
    if (!mounted_ || card_ == nullptr)
    {
        logger_.Error("Card not mounted, cannot write");
        return false;
    }
    esp_err_t ret = sdmmc_write_sectors(card_, src, start_sector, sector_count);
    if (ret != ESP_OK)
    {
        logger_.Error("WriteSectors failed (start=%zu, count=%zu): %s", start_sector, sector_count,
                      esp_err_to_name(ret));
    }
    return ret == ESP_OK;
}

bool SdSpi::EraseSectors(size_t start_sector, size_t sector_count, SdEraseMode mode) const
{
    if (!mounted_ || card_ == nullptr)
    {
        logger_.Error("Card not mounted, cannot erase");
        return false;
    }
    esp_err_t ret = sdmmc_erase_sectors(card_, start_sector, sector_count,
                                        static_cast<sdmmc_erase_arg_t>(mode));
    if (ret != ESP_OK)
    {
        logger_.Error("EraseSectors failed (start=%zu, count=%zu): %s", start_sector, sector_count,
                      esp_err_to_name(ret));
    }
    return ret == ESP_OK;
}

// ---------------------------------------------------------------------------
// 文件系统操作
// ---------------------------------------------------------------------------

bool SdSpi::Format()
{
    if (!mounted_ || card_ == nullptr)
    {
        logger_.Error("Card not mounted, cannot format");
        return false;
    }
    esp_err_t ret = esp_vfs_fat_sdcard_format(base_path_.c_str(), card_);
    if (ret != ESP_OK)
    {
        logger_.Error("Format failed: %s", esp_err_to_name(ret));
        return false;
    }
    logger_.Info("Formatted SD card at %s", base_path_.c_str());
    return true;
}

void SdSpi::PrintInfo() const
{
    if (!mounted_ || card_ == nullptr)
    {
        logger_.Warning("Card not mounted, cannot print info");
        return;
    }
    sdmmc_card_print_info(stdout, card_);
}

}  // namespace wrapper
