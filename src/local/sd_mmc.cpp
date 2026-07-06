#include "local/sd_mmc.hpp"
#include <cstdio>

namespace wrapper
{

SdMmc::SdMmc(Logger& logger) : logger_(logger), card_(nullptr), slot_(-1), mounted_(false) {}

SdMmc::~SdMmc() { Deinit(); }

// ---------------------------------------------------------------------------
// 生命周期
// ---------------------------------------------------------------------------

bool SdMmc::Init(int slot,
                 const SdMmcSlotConfig& slot_config,
                 const SdMmcMountConfig& mount_config,
                 std::string_view base_path)
{
    if (mounted_)
    {
        logger_.Warning("Already mounted. Deinitializing first.");
        Deinit();
    }

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.slot = slot;

    base_path_ = std::string(base_path);
    slot_ = slot;

    sdmmc_slot_config_t cfg = slot_config;

    esp_err_t ret = esp_vfs_fat_sdmmc_mount(base_path_.c_str(), &host, &cfg, &mount_config, &card_);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to mount SD (SDMMC, slot=%d): %s", slot, esp_err_to_name(ret));
        card_ = nullptr;
        mounted_ = false;
        slot_ = -1;
        base_path_.clear();
        return false;
    }

    mounted_ = true;
    logger_.Info("Mounted at %s (slot=%d)", base_path_.c_str(), slot);
    return true;
}

bool SdMmc::Deinit()
{
    if (!mounted_)
    {
        return true;
    }

    esp_err_t ret = esp_vfs_fat_sdcard_unmount(base_path_.c_str(), card_);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to unmount SD (SDMMC): %s", esp_err_to_name(ret));
        return false;
    }

    card_ = nullptr;
    mounted_ = false;
    slot_ = -1;
    logger_.Info("Unmounted from %s", base_path_.c_str());
    base_path_.clear();
    return true;
}

// ---------------------------------------------------------------------------
// 状态查询
// ---------------------------------------------------------------------------

std::optional<SdCardInfo> SdMmc::GetCardInfo() const
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

bool SdMmc::GetStatus() const
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

bool SdMmc::CanDiscard() const
{
    if (!mounted_ || card_ == nullptr)
        return false;
    return sdmmc_can_discard(card_) == ESP_OK;
}

bool SdMmc::CanTrim() const
{
    if (!mounted_ || card_ == nullptr)
        return false;
    return sdmmc_can_trim(card_) == ESP_OK;
}

// ---------------------------------------------------------------------------
// 总线运行时控制
// ---------------------------------------------------------------------------

bool SdMmc::SetBusWidth(uint8_t width)
{
    if (!mounted_ || slot_ < 0)
    {
        logger_.Error("Not mounted, cannot set bus width");
        return false;
    }
    esp_err_t ret = sdmmc_host_set_bus_width(slot_, static_cast<size_t>(width));
    if (ret != ESP_OK)
    {
        logger_.Error("SetBusWidth(%d) failed: %s", width, esp_err_to_name(ret));
    }
    return ret == ESP_OK;
}

bool SdMmc::SetClockFreq(uint32_t freq_khz)
{
    if (!mounted_ || slot_ < 0)
    {
        logger_.Error("Not mounted, cannot set clock freq");
        return false;
    }
    esp_err_t ret = sdmmc_host_set_card_clk(slot_, freq_khz);
    if (ret != ESP_OK)
    {
        logger_.Error("SetClockFreq(%u kHz) failed: %s", freq_khz, esp_err_to_name(ret));
    }
    return ret == ESP_OK;
}

// ---------------------------------------------------------------------------
// 扇区级 I/O
// ---------------------------------------------------------------------------

bool SdMmc::ReadSectors(size_t start_sector, size_t sector_count, void* dst) const
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

bool SdMmc::WriteSectors(size_t start_sector, size_t sector_count, const void* src) const
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

bool SdMmc::EraseSectors(size_t start_sector, size_t sector_count, SdEraseMode mode) const
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

bool SdMmc::Format()
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

void SdMmc::PrintInfo() const
{
    if (!mounted_ || card_ == nullptr)
    {
        logger_.Warning("Card not mounted, cannot print info");
        return;
    }
    sdmmc_card_print_info(stdout, card_);
}

// ---------------------------------------------------------------------------
// 测试
// ---------------------------------------------------------------------------

bool SdMmc::Test()
{
    if (!mounted_ || card_ == nullptr)
    {
        logger_.Error("Test: not mounted, call Init() first");
        return false;
    }

    auto info = GetCardInfo();
    if (info)
    {
        logger_.Info("Card: %s, Capacity: %.2f GB, Freq: %d kHz", info->name.c_str(),
                     static_cast<float>(info->capacity_bytes) / (1024.0f * 1024.0f * 1024.0f),
                     info->real_freq_khz);
    }

    const std::string test_path = base_path_ + "/test.txt";
    bool ok = true;

    // Write
    FILE* f = fopen(test_path.c_str(), "w");
    if (f)
    {
        fprintf(f, "Hello from SD card!");
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
