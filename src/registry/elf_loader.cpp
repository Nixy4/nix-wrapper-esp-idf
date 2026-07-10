#include "registry/elf_loader.hpp"

#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <string>
#include <vector>

#if __has_include("esp_elf.h")

namespace wrapper
{

namespace
{
constexpr const char* kSdCardMountPath = "/sdcard";

bool IsValidElfMagic(const uint8_t* data, size_t size)
{
    return data && size >= 4 && data[0] == 0x7f && data[1] == 'E' && data[2] == 'L' &&
           data[3] == 'F';
}
}  // namespace

// ─── ElfLoader ───────────────────────────────────────────────────────────────

ElfLoader::ElfLoader(Logger& logger) : logger_(logger) {}

ElfLoader::~ElfLoader()
{
    if (initialized_)
    {
        logger_.Warning("ElfLoader destroyed while still initialized; calling Deinit()");
        Deinit();
    }
}

// ── 生命周期 ──────────────────────────────────────────────────────────────────

bool ElfLoader::Init()
{
    if (initialized_)
    {
        logger_.Warning("Already initialized");
        return true;
    }

    int ret = esp_elf_init(&elf_);
    if (ret != 0)
    {
        logger_.Error("esp_elf_init failed (%d)", ret);
        return false;
    }

    initialized_ = true;
    loaded_ = false;
    logger_.Info("Initialized");
    return true;
}

void ElfLoader::Deinit()
{
    if (!initialized_)
        return;

    esp_elf_deinit(&elf_);
    initialized_ = false;
    loaded_ = false;
    logger_.Info("Deinitialized");
}

// ── 加载 ──────────────────────────────────────────────────────────────────────

bool ElfLoader::Load(const uint8_t* data, size_t size)
{
    if (!initialized_)
    {
        logger_.Error("Not initialized; call Init() first");
        return false;
    }

    if (!data || size == 0)
    {
        logger_.Error("Invalid ELF buffer (data=%p size=%zu)", static_cast<const void*>(data),
                      size);
        return false;
    }

    int ret = esp_elf_relocate(&elf_, data);
    if (ret != 0)
    {
        logger_.Error("esp_elf_relocate failed (%d)", ret);
        return false;
    }

    loaded_ = true;
    logger_.Info("ELF loaded and relocated (%zu bytes)", size);
    return true;
}

bool ElfLoader::LoadFromFile(const char* name) { return name ? LoadFromSpiffs(name) : false; }

bool ElfLoader::LoadFromSpiffs(std::string_view name)
{
    if (!initialized_)
    {
        logger_.Error("Not initialized; call Init() first");
        return false;
    }

    if (name.empty())
    {
        logger_.Error("SPIFFS filename is empty");
        return false;
    }

    std::string name_buf(name);

    elf_file_t file{};
    int ret = esp_elf_open(&file, name_buf.c_str());
    if (ret != 0)
    {
        logger_.Error("Failed to open SPIFFS ELF '%s' (%d)", name_buf.c_str(), ret);
        return false;
    }

    const size_t elf_size = file.size;
    ret = esp_elf_relocate(&elf_, file.payload);
    esp_elf_close(&file);

    if (ret != 0)
    {
        logger_.Error("esp_elf_relocate failed for SPIFFS ELF '%s' (%d)", name_buf.c_str(), ret);
        return false;
    }

    loaded_ = true;
    logger_.Info("SPIFFS ELF '%s' loaded (%zu bytes)", name_buf.c_str(), elf_size);
    return true;
}

bool ElfLoader::LoadFromSdCard(std::string_view file_name)
{
    if (file_name.empty())
    {
        logger_.Error("SDCard filename is empty");
        return false;
    }

    std::string full_path(kSdCardMountPath);
    if (full_path.back() != '/')
    {
        full_path.push_back('/');
    }
    full_path.append(file_name.data(), file_name.size());

    return LoadFromSdCardPath(full_path);
}

bool ElfLoader::LoadFromSdCardPath(std::string_view full_path)
{
    if (!initialized_)
    {
        logger_.Error("Not initialized; call Init() first");
        return false;
    }

    if (full_path.empty())
    {
        logger_.Error("SDCard ELF path is empty");
        return false;
    }

    std::string path(full_path);
    FILE* fp = fopen(path.c_str(), "rb");
    if (!fp)
    {
        logger_.Error("Failed to open SDCard ELF: %s (errno=%d)", path.c_str(), errno);
        return false;
    }

    if (fseek(fp, 0, SEEK_END) != 0)
    {
        logger_.Error("fseek end failed for %s", path.c_str());
        fclose(fp);
        return false;
    }

    long file_size = ftell(fp);
    if (file_size <= 0)
    {
        logger_.Error("Invalid ELF size (%ld) for %s", file_size, path.c_str());
        fclose(fp);
        return false;
    }

    if (fseek(fp, 0, SEEK_SET) != 0)
    {
        logger_.Error("fseek set failed for %s", path.c_str());
        fclose(fp);
        return false;
    }

    std::vector<uint8_t> elf_data(static_cast<size_t>(file_size));
    const size_t read_bytes = fread(elf_data.data(), 1, elf_data.size(), fp);
    fclose(fp);

    if (read_bytes != elf_data.size())
    {
        logger_.Error("Short read for %s: read=%zu expected=%zu", path.c_str(), read_bytes,
                      elf_data.size());
        return false;
    }

    if (!IsValidElfMagic(elf_data.data(), elf_data.size()))
    {
        logger_.Error("Invalid ELF magic in %s", path.c_str());
        return false;
    }

    if (!Load(elf_data.data(), elf_data.size()))
    {
        logger_.Error("Load failed for SDCard ELF path %s", path.c_str());
        return false;
    }

    logger_.Info("SDCard ELF loaded: %s (%zu bytes)", path.c_str(), elf_data.size());
    return true;
}

// ── 执行 ──────────────────────────────────────────────────────────────────────

bool ElfLoader::Run(int argc, char** argv, int opt)
{
    if (!loaded_)
    {
        logger_.Error("No ELF loaded; call Load() or LoadFromFile() first");
        return false;
    }

    int ret = esp_elf_request(&elf_, opt, argc, argv);
    if (ret != 0)
    {
        logger_.Error("esp_elf_request failed (%d)", ret);
        return false;
    }

    return true;
}

// ── 符号工具 ──────────────────────────────────────────────────────────────────

uintptr_t ElfLoader::MapSymbol(uintptr_t sym)
{
    if (!loaded_)
    {
        logger_.Error("No ELF loaded; cannot map symbol");
        return 0;
    }

    return esp_elf_map_sym(&elf_, sym);
}

// ── 调试辅助 ──────────────────────────────────────────────────────────────────

void ElfLoader::PrintElfHeader(const uint8_t* data) { esp_elf_print_ehdr(data); }

void ElfLoader::PrintProgramHeader(const uint8_t* data) { esp_elf_print_phdr(data); }

void ElfLoader::PrintSectionHeader(const uint8_t* data) { esp_elf_print_shdr(data); }

void ElfLoader::PrintSections()
{
    if (!loaded_)
    {
        logger_.Warning("No ELF loaded; nothing to print");
        return;
    }

    esp_elf_print_sec(&elf_);
}

// ── 全局符号表管理（静态） ────────────────────────────────────────────────────

bool ElfLoader::RegisterSymbols(esp_elf_symbol_table_t* symbol_table)
{
    return esp_elf_register_symbol(symbol_table) == 0;
}

bool ElfLoader::UnregisterSymbols(esp_elf_symbol_table_t* symbol_table)
{
    return esp_elf_unregister_symbol(symbol_table) == 0;
}

uintptr_t ElfLoader::FindSymbol(const char* sym_name) { return esp_elf_find_symbol(sym_name); }

void ElfLoader::SetSymbolResolver(symbol_resolver resolver) { elf_set_symbol_resolver(resolver); }

void ElfLoader::ResetSymbolResolver() { elf_reset_symbol_resolver(); }

}  // namespace wrapper

#endif  // __has_include("esp_elf.h")
