#pragma once

#if __has_include("esp_elf.h")

#include <cstddef>
#include <cstdint>

#include "esp_elf.h"

#include "local/logger.hpp"

namespace wrapper
{

/**
 * @brief ESP ELF Loader 封装类。
 *
 * 封装 espressif/elf_loader 组件，用于在运行时加载并执行
 * 针对 ESP32 系列 SoC 编译的 ELF 可执行文件或共享对象。
 *
 * 典型调用流程：
 * @code
 *   wrapper::ElfLoader loader(logger);
 *   loader.Init();
 *   loader.Load(elf_data, elf_size);   // 或 loader.LoadFromFile("app.elf")
 *   loader.Run(argc, argv);
 *   loader.Deinit();
 * @endcode
 */
class ElfLoader
{
    Logger& logger_;
    esp_elf_t elf_{};
    bool initialized_ = false;
    bool loaded_ = false;

   public:
    explicit ElfLoader(Logger& logger);
    ~ElfLoader();

    // 禁止拷贝（elf_ 内含由 esp_elf_init/deinit 管理的原始指针）
    ElfLoader(const ElfLoader&) = delete;
    ElfLoader& operator=(const ElfLoader&) = delete;

    // ── 生命周期 ──────────────────────────────────────────────────────────────

    /**
     * @brief 初始化 ELF 对象。必须在 Load() 前调用。
     * @return true 成功。
     */
    bool Init();

    /**
     * @brief 反初始化，释放 ELF loader 分配的所有资源。
     * 析构函数会自动调用此方法。
     */
    void Deinit();

    // ── 加载 ──────────────────────────────────────────────────────────────────

    /**
     * @brief 从原始内存缓冲区解码并重定位 ELF 镜像。
     *
     * @param data  指向 ELF 二进制数据的指针。
     * @param size  ELF 二进制数据的字节数。
     * @return true 成功。
     */
    bool Load(const uint8_t* data, size_t size);

    /**
     * @brief 从文件系统打开、重定位并关闭 ELF 文件。
     *
     * 实际路径由底层 esp_elf_open() 构造为 "<FS_PATH>/<name>"。
     *
     * @param name  ELF 文件名（不含路径）。
     * @return true 成功。
     */
    bool LoadFromFile(const char* name);

    // ── 执行 ──────────────────────────────────────────────────────────────────

    /**
     * @brief 执行已加载 ELF 的入口函数。
     *
     * @param argc  参数个数（默认 0）。
     * @param argv  参数数组（argc == 0 时可为 nullptr）。
     * @param opt   请求选项（默认 0）。
     * @return true 成功。
     */
    bool Run(int argc = 0, char** argv = nullptr, int opt = 0);

    // ── 符号工具 ──────────────────────────────────────────────────────────────

    /**
     * @brief 将 ELF 虚拟地址映射到运行时物理地址。
     *
     * @param sym  来自 ELF 文件的虚拟符号地址。
     * @return 物理地址；失败时返回 0。
     */
    uintptr_t MapSymbol(uintptr_t sym);

    // ── 调试辅助 ──────────────────────────────────────────────────────────────

    /** @brief 打印 ELF 文件头信息。 */
    void PrintElfHeader(const uint8_t* data);

    /** @brief 打印程序头信息。 */
    void PrintProgramHeader(const uint8_t* data);

    /** @brief 打印节头信息。 */
    void PrintSectionHeader(const uint8_t* data);

    /** @brief 打印已加载 ELF 的节信息。 */
    void PrintSections();

    // ── 状态查询 ──────────────────────────────────────────────────────────────

    bool IsInitialized() const { return initialized_; }
    bool IsLoaded() const { return loaded_; }

    // ── 全局符号表管理（静态） ────────────────────────────────────────────────

    /**
     * @brief 注册符号表，使被加载的 ELF 可以解析其中的符号。
     *
     * @param symbol_table  以 ESP_ELFSYM_END 结尾的 esp_elfsym 数组。
     * @return true 成功。
     *
     * @note 非线程安全；请在加载任何 ELF 之前调用，或使用外部锁。
     */
    static bool RegisterSymbols(esp_elf_symbol_table_t* symbol_table);

    /**
     * @brief 注销已注册的符号表。
     *
     * @param symbol_table  传入 RegisterSymbols() 的相同指针。
     * @return true 成功。
     *
     * @note 非线程安全。
     */
    static bool UnregisterSymbols(esp_elf_symbol_table_t* symbol_table);

    /**
     * @brief 在所有已注册符号表中按名称查找符号地址。
     *
     * @param sym_name  以 null 结尾的符号名字符串。
     * @return 符号地址；未找到时返回 0。
     */
    static uintptr_t FindSymbol(const char* sym_name);

    /**
     * @brief 覆盖全局符号解析函数。
     *
     * @param resolver  自定义解析器；其生命周期内指针必须保持有效。
     *
     * @warning 影响此后所有符号解析，全局生效，请谨慎使用。
     * @note 非线程安全。
     */
    static void SetSymbolResolver(symbol_resolver resolver);

    /** @brief 将符号解析器重置为内置默认实现。 */
    static void ResetSymbolResolver();
};

}  // namespace wrapper

#endif  // __has_include("esp_elf.h")