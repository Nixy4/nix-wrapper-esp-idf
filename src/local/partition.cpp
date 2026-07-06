#include "local/partition.hpp"

#include <algorithm>
#include <cinttypes>
#include <cstring>

#include "esp_flash.h"
#include "esp_flash_partitions.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_rom_md5.h"

using namespace wrapper;

namespace
{

// clang-format off
constexpr uint16_t kMagicEntry     = ESP_PARTITION_MAGIC;
constexpr uint16_t kMagicMd5       = ESP_PARTITION_MAGIC_MD5;
constexpr uint8_t  kTypeApp        = PART_TYPE_APP;
constexpr uint8_t  kTypeData       = PART_TYPE_DATA;
constexpr uint8_t  kSubtypeFactory = PART_SUBTYPE_FACTORY;
constexpr uint8_t  kSubtypeTest    = PART_SUBTYPE_TEST;
constexpr uint8_t  kSubtypeOtaFlag = PART_SUBTYPE_OTA_FLAG;
constexpr uint8_t  kSubtypeOtaMask = PART_SUBTYPE_OTA_MASK;
constexpr uint8_t  kSubtypeDataOta = PART_SUBTYPE_DATA_OTA;
// clang-format on

constexpr size_t kWriteChunkSize = 256;

static uint8_t sTableBuf[PartitionManager::kPartitionTableSize];
static uint8_t sVerifyBuf[PartitionManager::kPartitionTableSize];

uint16_t ReadU16(const uint8_t* p)
{
    return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

uint32_t ReadU32(const uint8_t* p)
{
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

void WriteU16(uint8_t* p, uint16_t value)
{
    p[0] = value & 0xFF;
    p[1] = (value >> 8) & 0xFF;
}

void WriteU32(uint8_t* p, uint32_t value)
{
    p[0] = value & 0xFF;
    p[1] = (value >> 8) & 0xFF;
    p[2] = (value >> 16) & 0xFF;
    p[3] = (value >> 24) & 0xFF;
}

uint32_t AlignUp(uint32_t value, uint32_t alignment)
{
    if (alignment == 0)
    {
        return value;
    }
    return (value + alignment - 1) & ~(alignment - 1);
}

bool CalcMd5(const uint8_t* data, size_t len, uint8_t* digest)
{
    md5_context_t ctx;
    esp_rom_md5_init(&ctx);
    esp_rom_md5_update(&ctx, data, static_cast<uint32_t>(len));
    esp_rom_md5_final(digest, &ctx);
    return true;
}

bool GetFlashSize(uint32_t& flash_size)
{
    return esp_flash_get_size(nullptr, &flash_size) == ESP_OK && flash_size > 0;
}

}  // namespace

// ─── PartitionEntry ──────────────────────────────────────────────────────────

bool PartitionEntry::IsApp() const { return type == kTypeApp; }

bool PartitionEntry::IsData() const { return type == kTypeData; }

bool PartitionEntry::IsOtaApp() const
{
    return IsApp() && (subtype & ~kSubtypeOtaMask) == kSubtypeOtaFlag;
}

bool PartitionEntry::IsFactoryApp() const
{
    return IsApp() && (subtype == kSubtypeFactory || subtype == kSubtypeTest);
}

// ─── PartitionManager ────────────────────────────────────────────────────────

PartitionManager::PartitionManager(Logger& logger) : logger_(logger) {}

bool PartitionManager::ReadCurrent(PartitionTable& table)
{
    esp_err_t err = esp_flash_read(
        nullptr, sTableBuf, kPartitionTableOffset, static_cast<uint32_t>(kPartitionTableSize));
    if (err != ESP_OK)
    {
        logger_.Error("Failed to read partition table from flash (err=0x%" PRIx32 ")",
                      static_cast<uint32_t>(err));
        return false;
    }
    return Parse(sTableBuf, kPartitionTableSize, table);
}

bool PartitionManager::Parse(const uint8_t* data, size_t size, PartitionTable& table)
{
    table = PartitionTable{};

    if (!data || size < kPartitionEntrySize)
    {
        logger_.Error("Partition table buffer too small");
        return false;
    }

    if (!GetFlashSize(table.flash_size))
    {
        logger_.Error("Failed to get flash size");
        return false;
    }

    for (size_t pos = 0; pos + kPartitionEntrySize <= size; pos += kPartitionEntrySize)
    {
        const uint8_t* raw = data + pos;
        const uint16_t magic = ReadU16(raw);

        if (magic == kMagicMd5)
        {
            table.has_md5 = true;
            if (pos + ESP_PARTITION_MD5_OFFSET + 16 > size)
            {
                logger_.Error("Partition table MD5 entry truncated");
                return false;
            }
            uint8_t digest[16] = {0};
            CalcMd5(data, pos, digest);
            if (memcmp(digest, raw + ESP_PARTITION_MD5_OFFSET, 16) != 0)
            {
                logger_.Error("Partition table MD5 mismatch");
                return false;
            }
            return true;
        }

        if (magic == 0xFFFF || magic == 0x0000)
        {
            return true;
        }

        if (magic != kMagicEntry)
        {
            logger_.Error("Invalid partition entry magic: 0x%04" PRIx16 " at offset 0x%x",
                          magic,
                          static_cast<unsigned>(pos));
            return false;
        }

        PartitionEntry entry{};
        entry.type = raw[2];
        entry.subtype = raw[3];
        entry.offset = ReadU32(raw + 4);
        entry.size = ReadU32(raw + 8);
        memcpy(entry.label, raw + 12, 16);
        entry.label[16] = '\0';
        entry.flags = ReadU32(raw + 28);
        table.entries.push_back(entry);
    }

    logger_.Error("Partition table terminator not found");
    return false;
}

bool PartitionManager::Build(const PartitionTable& table, uint8_t* out, size_t out_size)
{
    const size_t md5_entry_offset = table.entries.size() * kPartitionEntrySize;

    if (md5_entry_offset + kPartitionEntrySize > out_size)
    {
        logger_.Error("Partition table too large to fit in output buffer");
        return false;
    }

    memset(out, 0xFF, out_size);

    for (size_t i = 0; i < table.entries.size(); ++i)
    {
        const PartitionEntry& src = table.entries[i];
        uint8_t* dst = out + i * kPartitionEntrySize;
        WriteU16(dst, kMagicEntry);
        dst[2] = src.type;
        dst[3] = src.subtype;
        WriteU32(dst + 4, src.offset);
        WriteU32(dst + 8, src.size);
        memset(dst + 12, 0, 16);
        strncpy(reinterpret_cast<char*>(dst + 12), src.label, 15);
        WriteU32(dst + 28, src.flags);
    }

    // Write MD5 terminator entry
    uint8_t* md5_entry = out + md5_entry_offset;
    WriteU16(md5_entry, kMagicMd5);
    uint8_t digest[16] = {0};
    CalcMd5(out, md5_entry_offset, digest);
    memcpy(md5_entry + ESP_PARTITION_MD5_OFFSET, digest, 16);

    return true;
}

bool PartitionManager::Validate(const PartitionTable& table)
{
    uint32_t flash_size = table.flash_size;
    if (flash_size == 0 && !GetFlashSize(flash_size))
    {
        logger_.Error("Failed to get flash size for validation");
        return false;
    }

    if (table.entries.empty())
    {
        logger_.Error("Partition table has no entries");
        return false;
    }

    if ((table.entries.size() + 1) * kPartitionEntrySize > ESP_PARTITION_TABLE_MAX_LEN)
    {
        logger_.Error("Too many partition entries");
        return false;
    }

    for (size_t i = 0; i < table.entries.size(); ++i)
    {
        const PartitionEntry& e = table.entries[i];

        if (e.size == 0)
        {
            logger_.Error("Partition '%s' has zero size", e.label);
            return false;
        }

        if ((e.offset % kFlashSectorSize) != 0 || (e.size % kFlashSectorSize) != 0)
        {
            logger_.Error("Partition '%s' not sector-aligned (offset=0x%" PRIx32
                          " size=0x%" PRIx32 ")",
                          e.label,
                          e.offset,
                          e.size);
            return false;
        }

        if (e.offset < kPartitionTableOffset + kPartitionTableSize)
        {
            logger_.Error("Partition '%s' overlaps bootloader or partition table area", e.label);
            return false;
        }

        if (e.offset > flash_size || e.size > flash_size || e.offset + e.size > flash_size)
        {
            logger_.Error("Partition '%s' exceeds flash size (flash=0x%" PRIx32 ")",
                          e.label,
                          flash_size);
            return false;
        }

        if (e.label[0] == '\0')
        {
            logger_.Error("Partition has empty label");
            return false;
        }

        if (e.IsApp() && (e.offset % kAppPartitionAlignment) != 0)
        {
            logger_.Error("App partition '%s' not 64 KB-aligned (offset=0x%" PRIx32 ")",
                          e.label,
                          e.offset);
            return false;
        }

        for (size_t j = i + 1; j < table.entries.size(); ++j)
        {
            const PartitionEntry& other = table.entries[j];
            const uint32_t a_end = e.offset + e.size;
            const uint32_t b_end = other.offset + other.size;
            if (e.offset < b_end && other.offset < a_end)
            {
                logger_.Error("Partition '%s' and '%s' overlap", e.label, other.label);
                return false;
            }
            if (strncmp(e.label, other.label, 16) == 0)
            {
                logger_.Error("Duplicate partition label: '%s'", e.label);
                return false;
            }
        }
    }

    return true;
}

bool PartitionManager::Write(const PartitionTable& table)
{
    if (!Validate(table))
    {
        return false;
    }

    if (!Build(table, sTableBuf, kPartitionTableSize))
    {
        return false;
    }

    // Sanity-check the built table header
    if (sTableBuf[0] != 0xAA || sTableBuf[1] != 0x50)
    {
        logger_.Error("Built partition table has invalid magic");
        return false;
    }

    for (uint8_t attempt = 0; attempt < 3; ++attempt)
    {
        esp_err_t err = esp_flash_erase_region(
            nullptr, kPartitionTableOffset, static_cast<uint32_t>(kPartitionTableSize));
        if (err != ESP_OK)
        {
            logger_.Warning("Failed to erase partition table (attempt %d, err=0x%" PRIx32 ")",
                            attempt + 1,
                            static_cast<uint32_t>(err));
            continue;
        }

        bool write_ok = true;
        for (uint32_t offset = 0; offset < kPartitionTableSize; offset += kWriteChunkSize)
        {
            const uint32_t len =
                std::min(static_cast<uint32_t>(kWriteChunkSize), kPartitionTableSize - offset);
            err = esp_flash_write(
                nullptr, sTableBuf + offset, kPartitionTableOffset + offset, len);
            if (err != ESP_OK)
            {
                logger_.Warning(
                    "Failed to write partition table chunk at offset 0x%" PRIx32
                    " (attempt %d)",
                    offset,
                    attempt + 1);
                write_ok = false;
                break;
            }
        }

        if (!write_ok)
        {
            continue;
        }

        err = esp_flash_read(
            nullptr, sVerifyBuf, kPartitionTableOffset, static_cast<uint32_t>(kPartitionTableSize));
        if (err != ESP_OK || memcmp(sTableBuf, sVerifyBuf, kPartitionTableSize) != 0)
        {
            logger_.Warning("Partition table write verification failed (attempt %d)", attempt + 1);
            continue;
        }

        logger_.Info("Partition table written and verified successfully");
        return true;
    }

    logger_.Error("Failed to write partition table after 3 attempts");
    return false;
}

// ─── Query ────────────────────────────────────────────────────────────────────

PartitionEntry* PartitionManager::FindByLabel(PartitionTable& table, std::string_view label)
{
    for (PartitionEntry& e : table.entries)
    {
        if (strncmp(e.label, label.data(), 16) == 0)
        {
            return &e;
        }
    }
    return nullptr;
}

const PartitionEntry* PartitionManager::FindByLabel(const PartitionTable& table,
                                                    std::string_view label)
{
    for (const PartitionEntry& e : table.entries)
    {
        if (strncmp(e.label, label.data(), 16) == 0)
        {
            return &e;
        }
    }
    return nullptr;
}

PartitionEntry* PartitionManager::FindAppBySubtype(PartitionTable& table, uint8_t subtype)
{
    for (PartitionEntry& e : table.entries)
    {
        if (e.IsApp() && e.subtype == subtype)
        {
            return &e;
        }
    }
    return nullptr;
}

const PartitionEntry* PartitionManager::FindOtaData(const PartitionTable& table)
{
    for (const PartitionEntry& e : table.entries)
    {
        if (e.IsData() && e.subtype == kSubtypeDataOta)
        {
            return &e;
        }
    }
    return nullptr;
}

uint8_t PartitionManager::CountOtaApps(const PartitionTable& table)
{
    uint8_t count = 0;
    for (const PartitionEntry& e : table.entries)
    {
        if (e.IsOtaApp())
        {
            ++count;
        }
    }
    return count;
}

std::vector<PartitionRange> PartitionManager::GetFreeRanges(const PartitionTable& table)
{
    uint32_t flash_size = table.flash_size;
    if (flash_size == 0)
    {
        GetFlashSize(flash_size);
    }

    std::vector<PartitionEntry> sorted = table.entries;
    std::sort(
        sorted.begin(), sorted.end(), [](const PartitionEntry& a, const PartitionEntry& b) {
            return a.offset < b.offset;
        });

    std::vector<PartitionRange> free_ranges;
    uint32_t cursor = kPartitionTableOffset + kPartitionTableSize;

    for (const PartitionEntry& e : sorted)
    {
        if (e.offset > cursor)
        {
            free_ranges.push_back({cursor, e.offset - cursor});
        }
        cursor = e.offset + e.size;
    }

    if (flash_size > cursor)
    {
        free_ranges.push_back({cursor, flash_size - cursor});
    }

    return free_ranges;
}

bool PartitionManager::FindFreeRange(const PartitionTable& table,
                                     uint32_t required_size,
                                     uint32_t alignment,
                                     PartitionRange& range)
{
    for (const PartitionRange& r : GetFreeRanges(table))
    {
        const uint32_t aligned_start = AlignUp(r.offset, alignment);
        if (aligned_start + required_size <= r.offset + r.size)
        {
            range = {aligned_start, required_size};
            return true;
        }
    }
    logger_.Error("No free range of size 0x%" PRIx32 " (alignment 0x%" PRIx32 ") found",
                  required_size,
                  alignment);
    return false;
}

// ─── Modify ───────────────────────────────────────────────────────────────────

bool PartitionManager::AddEntry(PartitionTable& table, const PartitionEntry& entry)
{
    if ((table.entries.size() + 2) * kPartitionEntrySize > ESP_PARTITION_TABLE_MAX_LEN)
    {
        logger_.Error("Cannot add entry: partition table is full");
        return false;
    }
    table.entries.push_back(entry);
    return true;
}

bool PartitionManager::CreateOtaApp(PartitionTable& table,
                                    uint32_t image_size,
                                    std::string_view label,
                                    PartitionEntry* created)
{
    // Find the next unused OTA app subtype slot (ota_0 … ota_15)
    int next_subtype = -1;
    for (int i = 0; i <= 0x0F; ++i)
    {
        const uint8_t candidate = static_cast<uint8_t>(kSubtypeOtaFlag | i);
        bool in_use = false;
        for (const PartitionEntry& e : table.entries)
        {
            if (e.IsOtaApp() && e.subtype == candidate)
            {
                in_use = true;
                break;
            }
        }
        if (!in_use)
        {
            next_subtype = candidate;
            break;
        }
    }

    if (next_subtype < 0)
    {
        logger_.Error("No available OTA app subtype slot");
        return false;
    }

    const uint32_t aligned_size = AlignUp(image_size, kAppPartitionAlignment);
    PartitionRange range{};
    if (!FindFreeRange(table, aligned_size, kAppPartitionAlignment, range))
    {
        return false;
    }

    PartitionEntry entry{};
    entry.type = kTypeApp;
    entry.subtype = static_cast<uint8_t>(next_subtype);
    entry.offset = range.offset;
    entry.size = aligned_size;
    entry.flags = 0;
    memset(entry.label, 0, sizeof(entry.label));
    memcpy(entry.label, label.data(), std::min(label.size(), size_t(16)));

    if (!AddEntry(table, entry))
    {
        return false;
    }

    if (created)
    {
        *created = entry;
    }

    logger_.Info("Created OTA app '%s': offset=0x%" PRIx32 " size=0x%" PRIx32 " subtype=0x%02x",
                 entry.label,
                 entry.offset,
                 entry.size,
                 entry.subtype);
    return true;
}

bool PartitionManager::CreateDataPartition(PartitionTable& table,
                                           uint8_t subtype,
                                           std::string_view label,
                                           uint32_t size,
                                           PartitionEntry* created)
{
    const uint32_t alignment = GetAlignment(kTypeData, subtype);
    const uint32_t aligned_size = AlignUp(size, kFlashSectorSize);

    PartitionRange range{};
    if (!FindFreeRange(table, aligned_size, alignment, range))
    {
        return false;
    }

    PartitionEntry entry{};
    entry.type = kTypeData;
    entry.subtype = subtype;
    entry.offset = range.offset;
    entry.size = aligned_size;
    entry.flags = 0;
    memset(entry.label, 0, sizeof(entry.label));
    memcpy(entry.label, label.data(), std::min(label.size(), size_t(16)));

    if (!AddEntry(table, entry))
    {
        return false;
    }

    if (created)
    {
        *created = entry;
    }

    logger_.Info("Created data partition '%s': offset=0x%" PRIx32 " size=0x%" PRIx32
                 " subtype=0x%02x",
                 entry.label,
                 entry.offset,
                 entry.size,
                 entry.subtype);
    return true;
}

bool PartitionManager::Compact(PartitionTable& table)
{
    uint32_t flash_size = table.flash_size;
    if (flash_size == 0 && !GetFlashSize(flash_size))
    {
        logger_.Error("Failed to get flash size for compact");
        return false;
    }
    table.flash_size = flash_size;

    std::sort(
        table.entries.begin(), table.entries.end(), [](const PartitionEntry& a, const PartitionEntry& b) {
            return a.offset < b.offset;
        });

    uint32_t cursor = kPartitionTableOffset + kPartitionTableSize;
    for (PartitionEntry& e : table.entries)
    {
        cursor = AlignUp(cursor, GetAlignment(e.type, e.subtype));
        if (cursor > flash_size || e.size > flash_size || cursor + e.size > flash_size)
        {
            logger_.Error("Compacted partition table exceeds flash size");
            return false;
        }
        e.offset = cursor;
        cursor += e.size;
    }

    return Validate(table);
}

// ─── OTA boot control ─────────────────────────────────────────────────────────

bool PartitionManager::SetOtaBoot(const PartitionTable& table, uint8_t app_subtype)
{
    const PartitionEntry* target = nullptr;
    for (const PartitionEntry& e : table.entries)
    {
        if (e.IsApp() && e.subtype == app_subtype)
        {
            target = &e;
            break;
        }
    }

    if (!target)
    {
        logger_.Error("App partition with subtype 0x%02x not found", app_subtype);
        return false;
    }

    const esp_partition_t* part = esp_partition_find_first(
        static_cast<esp_partition_type_t>(target->type),
        static_cast<esp_partition_subtype_t>(target->subtype),
        target->label);
    if (!part)
    {
        logger_.Error("Could not locate partition '%s' via esp_partition API", target->label);
        return false;
    }

    const esp_err_t err = esp_ota_set_boot_partition(part);
    if (err != ESP_OK)
    {
        logger_.Error("esp_ota_set_boot_partition failed for '%s' (err=0x%" PRIx32 ")",
                      target->label,
                      static_cast<uint32_t>(err));
        return false;
    }

    logger_.Info("OTA boot target set to '%s' (subtype=0x%02x)", target->label, app_subtype);
    return true;
}

bool PartitionManager::ClearOtaBoot(const PartitionTable& table)
{
    const PartitionEntry* factory = nullptr;
    for (const PartitionEntry& e : table.entries)
    {
        if (e.IsFactoryApp())
        {
            factory = &e;
            break;
        }
    }

    if (!factory)
    {
        logger_.Error("No factory app partition found to reset OTA boot");
        return false;
    }

    const esp_partition_t* part = esp_partition_find_first(
        static_cast<esp_partition_type_t>(factory->type),
        static_cast<esp_partition_subtype_t>(factory->subtype),
        factory->label);
    if (!part)
    {
        logger_.Error("Could not locate factory partition via esp_partition API");
        return false;
    }

    const esp_err_t err = esp_ota_set_boot_partition(part);
    if (err != ESP_OK)
    {
        logger_.Error("esp_ota_set_boot_partition (factory reset) failed (err=0x%" PRIx32 ")",
                      static_cast<uint32_t>(err));
        return false;
    }

    logger_.Info("OTA boot cleared; factory partition '%s' set as boot target", factory->label);
    return true;
}

// ─── Utilities ────────────────────────────────────────────────────────────────

uint32_t PartitionManager::GetAlignment(uint8_t type, uint8_t subtype)
{
    if (type == kTypeApp)
    {
        return kAppPartitionAlignment;
    }
    if (type == kTypeData &&
        (subtype == ESP_PARTITION_SUBTYPE_DATA_FAT ||
         subtype == ESP_PARTITION_SUBTYPE_DATA_SPIFFS ||
         subtype == ESP_PARTITION_SUBTYPE_DATA_LITTLEFS))
    {
        return kAppPartitionAlignment;
    }
    return kFlashSectorSize;
}

std::string PartitionManager::FormatHexSize(uint32_t value)
{
    char buf[11] = {0};
    snprintf(buf, sizeof(buf), "0x%08" PRIX32, value);
    return std::string(buf);
}

std::string PartitionManager::FormatHumanSize(uint32_t value)
{
    constexpr uint32_t kMb = 1024U * 1024U;
    constexpr uint32_t kKb = 1024U;
    if ((value % kMb) == 0)
    {
        return std::to_string(value / kMb) + "MB";
    }
    if ((value % kKb) == 0)
    {
        return std::to_string(value / kKb) + "KB";
    }
    return std::to_string(value) + "B";
}
