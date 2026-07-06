#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include "local/logger.hpp"

namespace wrapper
{

struct PartitionEntry
{
    uint8_t type = 0xFF;
    uint8_t subtype = 0xFF;
    uint32_t offset = 0;
    uint32_t size = 0;
    uint32_t flags = 0;
    char label[17] = {0};

    bool IsApp() const;
    bool IsData() const;
    bool IsOtaApp() const;
    bool IsFactoryApp() const;
};

struct PartitionTable
{
    std::vector<PartitionEntry> entries;
    uint32_t flash_size = 0;
    bool has_md5 = false;
};

struct PartitionRange
{
    uint32_t offset = 0;
    uint32_t size = 0;
};

class PartitionManager
{
    Logger& logger_;

   public:
    static constexpr uint32_t kPartitionTableOffset = 0x8000;
    static constexpr uint32_t kPartitionTableSize = 0x1000;
    static constexpr uint32_t kFlashSectorSize = 0x1000;
    static constexpr uint32_t kAppPartitionAlignment = 0x10000;
    static constexpr size_t kPartitionEntrySize = 0x20;

    explicit PartitionManager(Logger& logger);

    // Read / Parse / Build / Write
    bool ReadCurrent(PartitionTable& table);
    bool Parse(const uint8_t* data, size_t size, PartitionTable& table);
    bool Build(const PartitionTable& table, uint8_t* out, size_t out_size);
    bool Validate(const PartitionTable& table);
    bool Write(const PartitionTable& table);

    // Query
    PartitionEntry* FindByLabel(PartitionTable& table, std::string_view label);
    const PartitionEntry* FindByLabel(const PartitionTable& table, std::string_view label);
    PartitionEntry* FindAppBySubtype(PartitionTable& table, uint8_t subtype);
    const PartitionEntry* FindOtaData(const PartitionTable& table);
    uint8_t CountOtaApps(const PartitionTable& table);
    std::vector<PartitionRange> GetFreeRanges(const PartitionTable& table);
    bool FindFreeRange(const PartitionTable& table,
                       uint32_t required_size,
                       uint32_t alignment,
                       PartitionRange& range);

    // Modify
    bool AddEntry(PartitionTable& table, const PartitionEntry& entry);
    bool CreateOtaApp(PartitionTable& table,
                      uint32_t image_size,
                      std::string_view label,
                      PartitionEntry* created = nullptr);
    bool CreateDataPartition(PartitionTable& table,
                             uint8_t subtype,
                             std::string_view label,
                             uint32_t size,
                             PartitionEntry* created = nullptr);
    bool Compact(PartitionTable& table);

    // OTA boot control
    bool SetOtaBoot(const PartitionTable& table, uint8_t app_subtype);
    bool ClearOtaBoot(const PartitionTable& table);

    // Utilities
    static uint32_t GetAlignment(uint8_t type, uint8_t subtype);
    static std::string FormatHexSize(uint32_t value);
    static std::string FormatHumanSize(uint32_t value);
};

}  // namespace wrapper
