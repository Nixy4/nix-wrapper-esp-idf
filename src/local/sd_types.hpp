#pragma once

#include <cstdint>
#include <string>

namespace wrapper
{

/**
 * @brief SD/MMC 卡属性信息（C++ 风格封装自 sdmmc_card_t）
 */
struct SdCardInfo
{
    std::string name;           ///< 卡产品名称（来自 CID 寄存器，最长 7 字节）
    uint64_t    capacity_bytes; ///< 总容量（字节）
    uint32_t    sector_count;   ///< 扇区总数
    uint32_t    sector_size;    ///< 每扇区字节数（通常 512）
    uint32_t    max_freq_khz;   ///< 卡支持的最大时钟频率（kHz）
    int         real_freq_khz;  ///< 当前实际时钟频率（kHz）
    bool        is_mmc;         ///< 是否为 eMMC / MMC 卡
    bool        is_sdio;        ///< 是否为 SDIO 卡
};

/**
 * @brief SD 卡擦除模式（映射自 sdmmc_erase_arg_t）
 */
enum class SdEraseMode : int
{
    Erase   = 0,  ///< 物理擦除（对应 SDMMC_ERASE_ARG / MMC Trim）
    Discard = 1,  ///< 逻辑丢弃（对应 SDMMC_DISCARD_ARG）
};

}  // namespace wrapper
