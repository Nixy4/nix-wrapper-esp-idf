# nix-wrapper-esp-idf 编程风格与设计思路

## 一、设计目标

`nix-wrapper-esp-idf` 是对 ESP-IDF 框架的 C++ 风格二次封装，核心动机如下：

- ESP-IDF 的 `xxx_config_t` 结构体嵌套层次深、字段多，填参繁琐；封装后统一在 `XxxConfig` 构造函数中一次性完成。
- 部分外设初始化流程步骤繁多；适当封装，减少应用层样板代码。
- 提供统一的日志、错误处理和资源管理约定，便于多人协作与阅读。

---

## 二、分层架构

```
board/          ← 板级单例：聚合所有总线与外设的初始化
  lilygo/
  m5stack/
  seeed-studio/
device/         ← 具体芯片驱动：建立在 wrapper 之上
wrapper/        ← ESP-IDF 组件封装：总线、中间件、软件模块
misc/           ← 第三方 HAL 适配（如 u8g2）
```

### 2.1 wrapper 层

对 ESP-IDF 本体组件的 C++ 类封装，包括：

| 文件 | 封装内容 |
|------|----------|
| `i2c.hpp/cpp` | I2C 主机总线与设备 |
| `spi.hpp/cpp` | SPI 主机总线与设备 |
| `i2s.hpp/cpp` | I2S 总线 |
| `display.hpp/cpp` | LCD panel IO（I2C/SPI 两路）|
| `lvgl.hpp/cpp` | esp_lvgl_port 移植层 |
| `audio.hpp/cpp` | esp_codec_dev 音频编解码器 |
| `freertos.hpp/cpp` | Task / Queue / Semaphore / EventGroup |
| `logger.hpp/cpp` | ESP_LOG 多标签日志 |
| `ota.hpp/cpp` | OTA 固件升级 |
| `soc.hpp/cpp` | NVS、EventLoop、WiFi |
| `encoder.hpp/cpp` | GPIO 正交旋转编码器 |
| `ledc.hpp/cpp` | LEDC PWM（背光控制）|
| `fatfs/spiffs/sd_spi/sd_mmc` | 文件系统挂载 |

### 2.2 device 层

基于 `wrapper` 的具体外设驱动，继承 `I2cDevice` / `SpiDevice`：

- `Xl9555`、`Aw9523`、`Pi4ioe5v6408`：I²C GPIO 扩展器
- `St7796`、`Ili9341`、`Ssd1306`：LCD 驱动
- `Tca8418`、`M5stackCardputerKeyboard`：键盘矩阵
- `Bq25896`、`Axp2101`、`Ip5306`：PMU / 充电管理
- `Gt911`、`LcdTouchFt5x06`：触摸控制器

### 2.3 board 层

每块开发板对应一个单例类，聚合该板全部总线与外设。
应用层直接通过 board 单例访问硬件，无需关心底层初始化细节。

---

## 三、命名约定

| 元素 | 风格 | 示例 |
|------|------|------|
| 类、结构体 | PascalCase | `I2cBus`、`SpiBusConfig`、`LilyGoLoraPager` |
| 公有方法 | PascalCase | `Init()`、`GetLogger()`、`SetLevel()` |
| 私有成员变量 | snake_case + 尾下划线 | `logger_`、`bus_handle_`、`initialized_` |
| 局部 static constexpr 常量 | `k` 前缀 + PascalCase | `kXl9555PinSdEn`、`kEs8311Addr` |
| 类内 static constexpr 常量 | UPPER_SNAKE_CASE | `DEFAULT_ADDR`、`REG_INPUT_P0`、`CFG_AI` |
| 文件名 | 全小写 + 连字符 | `t-lora-pager.hpp`、`display-dsi.cpp` |
| 命名空间 | 全小写 | `wrapper` |

---

## 四、Config 结构体模式

所有 ESP-IDF 配置结构体均被包装成带具名参数的 C++ 构造函数。

**原则**：

- `XxxBusConfig` 继承或组合 ESP-IDF 的 `xxx_bus_config_t`，在构造函数中按字段逐一赋值。
- 所有参数提供合理默认值，只需覆盖与板子相关的部分。
- 构造函数不做 IO 操作，仅填充配置数据。

```cpp
// 示例：I2cBusConfig
I2cBusConfig i2c_bus_cfg(
    I2C_NUM_0,           // port
    GPIO_NUM_3,          // sda
    GPIO_NUM_2,          // scl
    I2C_CLK_SRC_DEFAULT, // clk_src
    7,                   // glitch_ignore_cnt
    0,                   // intr_priority
    0,                   // trans_queue_depth
    true,                // enable_internal_pullup
    false                // allow_pd
);
```

---

## 五、Logger 注入模式

所有类通过构造函数接受 `Logger&` 引用，而非内部创建 Logger 实例。

**Logger 特性**：

- 封装 `ESP_LOG`，支持多标签格式（`| tag1 | tag2 |`）。
- 方法：`Verbose` / `Debug` / `Info` / `Warning` / `Error` / `Fatal`。
- 可通过 `SetTags()` 在运行时动态修改标签。
- 支持变参模板，调用方式与 `printf` 一致。

```cpp
// 类持有 Logger 引用
class I2cBus {
    Logger& logger_;
public:
    I2cBus(Logger& logger);
};

// board 层为每个子模块创建独立 Logger
Logger i2c_logger("I2cBus");
I2cBus i2c_bus(i2c_logger);
```

---

## 六、Init / Deinit 资源管理模式

组件不在构造函数中分配硬件资源，使用显式的 `Init()` / `Deinit()` 方法。

**原则**：

- 构造函数只存储引用、初始化成员为 `nullptr` / 默认值。
- `Init()` 执行实际的硬件初始化，返回 `bool`。
- 析构函数调用 `Deinit()`，实现安全的资源释放。
- `Init()` 具有幂等保护：若已初始化则先 `Deinit()` 再重新初始化。

```cpp
bool I2cBus::Init(const I2cBusConfig& config)
{
    if (bus_handle_ != nullptr) {
        logger_.Warning("Already initialized. Deinitializing first.");
        Deinit();
    }
    esp_err_t ret = i2c_new_master_bus(&config, &bus_handle_);
    if (ret != ESP_OK) {
        logger_.Error("Failed: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}
```

---

## 七、错误处理约定

- **所有操作均返回 `bool`**：`true` = 成功，`false` = 失败。
- **不使用异常**（ESP-IDF 默认不启用 C++ 异常）。
- 错误原因通过 `Logger` 输出，不向上层抛出异常或设置全局错误码。
- ESP-IDF 返回的 `esp_err_t` 通过 `esp_err_to_name()` 转为可读字符串记录到日志。

---

## 八、Board 单例模式

每块开发板对应一个单例，采用 Meyer's Singleton（局部静态变量）：

```cpp
class LilyGoLoraPager {
    LilyGoLoraPager() = default;
    // 禁止拷贝与移动
    LilyGoLoraPager(const LilyGoLoraPager&) = delete;
    LilyGoLoraPager& operator=(const LilyGoLoraPager&) = delete;
public:
    static LilyGoLoraPager& GetInstance() {
        static LilyGoLoraPager instance;
        return instance;
    }
};
```

**初始化约定**：

- 按依赖顺序调用多个 `InitXxx()` 方法，而非一个大 `Init()`。
- 每个 `InitXxx()` 在注释或文档中标注前置依赖。
- 提供 `GetXxx()` 方法暴露各子组件引用，应用层通过 board 单例访问一切硬件。

---

## 九、device 层驱动约定

继承自 `I2cDevice` / `SpiDevice` 的驱动类遵循以下约定：

- 在类中以 `static constexpr` 声明默认地址（`DEFAULT_ADDR`）和速度（`DEFAULT_SPEED`）。
- 寄存器地址命名为 `REG_xxx`，配置位掩码命名为 `CFG_xxx` / `INT_xxx`。
- 公有方法以动词短语命名，如 `SetLevel()`、`GetLevel()`、`SetDirection()`。
- 驱动头文件包含简短的 Doxygen 注释，说明芯片型号、功能和默认地址。

```cpp
class Xl9555 {
public:
    static constexpr uint8_t DEFAULT_ADDR = 0x20;
    static constexpr uint8_t REG_OUTPUT_P0 = 0x02;
    static constexpr uint32_t DIR_OUTPUT = 0;

    bool SetLevel(uint32_t io_num, uint32_t level);
    bool GetLevel(uint32_t io_num, uint32_t* level);
};
```

---

## 十、线程安全约定

- 需要跨任务共享的状态使用 `std::atomic`（如 `shutdown_requested`、编码器 delta/click）。
- 驱动本身不内置互斥锁；调用方负责并发保护（LVGL port 层例外，内部有锁）。
- GPIO ISR 回调仅通过 `std::atomic` 写入标志，不做任何阻塞或内存分配。

---

## 十一、现代 C++ 使用规范

| 特性 | 用途 |
|------|------|
| `#pragma once` | 头文件保护 |
| `std::string_view` | 只读字符串参数 |
| `std::function` | 回调参数（如 LCD 初始化回调）|
| `std::atomic<T>` | 跨任务原子标志 |
| `std::optional` | 可选返回值 |
| `template<typename T>` | Queue<T>、Logger variadic tags |
| 折叠表达式 `(expr, ...)` | Logger::SetTags 可变参数展开 |
| `static_cast` | 类型转换，禁止 C 风格强转 |

---

## 十二、Kconfig 与条件编译

- 板级选择通过 `Kconfig.projbuild` 的 `choice WRAPPER_ESP32_BOARD` 控制。
- `CMakeLists.txt` 根据 `IDF_TARGET` 和 `CONFIG_WRAPPER_ESP32_BOARD_xxx` 动态选择编译文件。
- 板级特有代码用 `#if CONFIG_WRAPPER_ESP32_BOARD_xxx ... #endif` 包裹。
- DSI 显示和 ILI9881C 等平台专属模块从通用源列表中排除，按需追加。

---

## 十三、典型调用流程（以 board 层为例）

```cpp
// 应用层 app-main.cpp
auto& board = wrapper::LilyGoLoraPager::GetInstance();

board.InitCoreBusAndIoExpander();  // I2C + XL9555 + BQ25896
board.InitDisplay();               // SPI + ST7796 + LEDC + LVGL
board.GetLvglPort().SetRotation(LV_DISPLAY_ROTATION_0);
board.InitAudio();                 // I2S + ES8311
board.InitKeyboard();              // TCA8418
board.InitEncoder();               // GPIO 编码器 + LVGL 输入设备
board.InitSdCard();                // SPI SD + FATFS /sdcard

// 访问子组件
board.GetDisplay();                // St7796&
board.GetIoExpander();             // Xl9555&
board.SetDisplayBrightness(80);    // 便捷方法
```
