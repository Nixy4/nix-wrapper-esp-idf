#include "registry/elf_loader.hpp"

#include <cinttypes>

#if __has_include("esp_elf.h")

namespace wrapper
{

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

bool ElfLoader::LoadFromFile(const char* name)
{
    if (!initialized_)
    {
        logger_.Error("Not initialized; call Init() first");
        return false;
    }

    if (!name)
    {
        logger_.Error("Filename is null");
        return false;
    }

    elf_file_t file{};
    int ret = esp_elf_open(&file, name);
    if (ret != 0)
    {
        logger_.Error("Failed to open ELF file '%s' (%d)", name, ret);
        return false;
    }

    const size_t elf_size = file.size;
    ret = esp_elf_relocate(&elf_, file.payload);
    esp_elf_close(&file);

    if (ret != 0)
    {
        logger_.Error("esp_elf_relocate failed for '%s' (%d)", name, ret);
        return false;
    }

    loaded_ = true;
    logger_.Info("ELF '%s' loaded from filesystem (%zu bytes)", name, elf_size);
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
