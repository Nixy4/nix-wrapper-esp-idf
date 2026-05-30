#include "wrapper/console.hpp"

#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include "esp_system.h"
#include "linenoise/linenoise.h"

#if defined(CONFIG_ESP_CONSOLE_USB_CDC)
#include "esp_vfs_cdcacm.h"
#endif

namespace wrapper
{

namespace
{
constexpr uint32_t kStopPollIntervalMs = 50;
constexpr uint32_t kStopTimeoutMs = 2000;
constexpr uint32_t kTaskStackSize = 4096;
constexpr UBaseType_t kTaskPriority = 5;
constexpr const char* kDefaultPrompt = "esp>";
}  // namespace

Console::Console() = default;

Console::~Console()
{
    if (initialized_)
    {
        Deinit();
    }
}

bool Console::InitPeripheral(const ConsolePeripheralConfigUart& uart_config)
{
    fflush(stdout);
    fsync(fileno(stdout));

#ifdef CONFIG_ESP_CONSOLE_UART_NUM
    if (uart_config.uart_num != CONFIG_ESP_CONSOLE_UART_NUM)
    {
        logger_.Warning(
            "uart_num=%d differs from CONFIG_ESP_CONSOLE_UART_NUM=%d; stdio may go to a different "
            "port than linenoise reads",
            static_cast<int>(uart_config.uart_num), static_cast<int>(CONFIG_ESP_CONSOLE_UART_NUM));
    }
#endif

    uart_vfs_dev_port_set_rx_line_endings(uart_config.uart_num, ESP_LINE_ENDINGS_CR);
    uart_vfs_dev_port_set_tx_line_endings(uart_config.uart_num, ESP_LINE_ENDINGS_CRLF);

    esp_err_t err =
        uart_driver_install(uart_config.uart_num, uart_config.rx_buffer_size, 0, 0, nullptr, 0);
    if (err != ESP_OK)
    {
        logger_.Error("uart_driver_install failed: %s", esp_err_to_name(err));
        return false;
    }

    err = uart_param_config(uart_config.uart_num, static_cast<const uart_config_t*>(&uart_config));
    if (err != ESP_OK)
    {
        logger_.Error("uart_param_config failed: %s", esp_err_to_name(err));
        uart_driver_delete(uart_config.uart_num);
        return false;
    }

    uart_vfs_dev_use_driver(uart_config.uart_num);
    setvbuf(stdin, nullptr, _IONBF, 0);

    uart_port_ = uart_config.uart_num;
    backend_ = Backend::Uart;
    return true;
}

bool Console::InitPeripheral(const ConsolePeripheralConfigUsbCdc& usb_cdc_config)
{
    fflush(stdout);
    fsync(fileno(stdout));

#if defined(CONFIG_ESP_CONSOLE_USB_CDC)
    esp_vfs_dev_cdcacm_set_rx_line_endings(usb_cdc_config.rx_line_endings);
    esp_vfs_dev_cdcacm_set_tx_line_endings(usb_cdc_config.tx_line_endings);

    fcntl(fileno(stdout), F_SETFL, 0);
    fcntl(fileno(stdin), F_SETFL, 0);
    setvbuf(stdin, nullptr, _IONBF, 0);

    backend_ = Backend::UsbCdc;
    return true;
#else
    (void)usb_cdc_config;
    logger_.Error("USB CDC console not enabled in sdkconfig");
    return false;
#endif
}

bool Console::InitPeripheral(const ConsolePeripheralConfigUsbSerialJtag& usb_serial_jtag_config)
{
    fflush(stdout);
    fsync(fileno(stdout));

    usb_serial_jtag_vfs_set_rx_line_endings(usb_serial_jtag_config.rx_line_endings);
    usb_serial_jtag_vfs_set_tx_line_endings(usb_serial_jtag_config.tx_line_endings);

    fcntl(fileno(stdout), F_SETFL, 0);
    fcntl(fileno(stdin), F_SETFL, 0);

    usb_serial_jtag_driver_config_t local_cfg = usb_serial_jtag_config;
    esp_err_t err = usb_serial_jtag_driver_install(&local_cfg);
    if (err != ESP_OK)
    {
        logger_.Error("usb_serial_jtag_driver_install failed: %s", esp_err_to_name(err));
        return false;
    }

    usb_serial_jtag_vfs_use_driver();
    setvbuf(stdin, nullptr, _IONBF, 0);

    backend_ = Backend::UsbSerialJtag;
    return true;
}

bool Console::DeinitPeripheral()
{
    switch (backend_)
    {
        case Backend::Uart:
        {
            esp_err_t err = uart_driver_delete(uart_port_);
            if (err != ESP_OK)
            {
                logger_.Warning("uart_driver_delete failed: %s", esp_err_to_name(err));
            }
            break;
        }
        case Backend::UsbSerialJtag:
        {
            esp_err_t err = usb_serial_jtag_driver_uninstall();
            if (err != ESP_OK)
            {
                logger_.Warning("usb_serial_jtag_driver_uninstall failed: %s",
                                esp_err_to_name(err));
            }
            break;
        }
        case Backend::UsbCdc:
        case Backend::None:
        default:
            break;
    }
    backend_ = Backend::None;
    return true;
}

bool Console::InitConsole(const ConsoleConfig& console_config)
{
    esp_err_t err = esp_console_init(static_cast<const esp_console_config_t*>(&console_config));
    if (err != ESP_OK)
    {
        logger_.Error("esp_console_init failed: %s", esp_err_to_name(err));
        return false;
    }

    linenoiseSetMultiLine(1);
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback(reinterpret_cast<linenoiseHintsCallback*>(&esp_console_get_hint));
    linenoiseHistorySetMaxLen(console_config.history_max_len);
    linenoiseSetMaxLineLen(console_config.max_cmdline_length);
    linenoiseAllowEmpty(false);

    if (linenoiseProbe() != 0)
    {
        linenoiseSetDumbMode(1);
    }
    return true;
}

bool Console::DeinitConsole()
{
    esp_err_t err = esp_console_deinit();
    if (err != ESP_OK)
    {
        logger_.Warning("esp_console_deinit failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

void Console::SetupPrompt(const char* prompt)
{
    const char* base = (prompt != nullptr && prompt[0] != '\0') ? prompt : kDefaultPrompt;
    int written;
    if (linenoiseIsDumbMode())
    {
        written = std::snprintf(prompt_, sizeof(prompt_), "%s ", base);
    }
    else
    {
        written =
            std::snprintf(prompt_, sizeof(prompt_), "%s%s %s", LOG_COLOR_I, base, LOG_RESET_COLOR);
    }
    if (written < 0 || static_cast<size_t>(written) >= sizeof(prompt_))
    {
        logger_.Warning("prompt truncated to %u bytes", static_cast<unsigned>(sizeof(prompt_) - 1));
    }
}

bool Console::FinalizeInit(const ConsoleConfig& console_config)
{
    if (!InitConsole(console_config))
    {
        DeinitPeripheral();
        return false;
    }
    logger_.Info("console library ready (max_cmdline=%u, history_max=%u, dumb=%d)",
                 static_cast<unsigned>(console_config.max_cmdline_length),
                 static_cast<unsigned>(console_config.history_max_len),
                 static_cast<int>(linenoiseIsDumbMode()));
    SetupPrompt(console_config.prompt);

    history_path_ = console_config.history_save_path;
    if (history_path_ != nullptr && history_path_[0] != '\0')
    {
        int rc = linenoiseHistoryLoad(history_path_);
        if (rc != 0)
        {
            logger_.Warning("linenoiseHistoryLoad('%s') failed (rc=%d); continuing without history",
                            history_path_, rc);
        }
        else
        {
            logger_.Info("history loaded from '%s'", history_path_);
        }
    }
    else
    {
        history_path_ = nullptr;
    }

    initialized_ = true;
    return true;
}

bool Console::Init(const ConsolePeripheralConfigUart& uart_config,
                   const ConsoleConfig& console_config)
{
    if (initialized_)
    {
        logger_.Warning("already initialized");
        return false;
    }
    logger_.Info("Init(UART, port=%d, baud=%d)", static_cast<int>(uart_config.uart_num),
                 static_cast<int>(uart_config.baud_rate));
    if (!InitPeripheral(uart_config))
    {
        return false;
    }
    bool ok = FinalizeInit(console_config);
    if (ok)
        logger_.Info("Init done (backend=UART)");
    return ok;
}

bool Console::Init(const ConsolePeripheralConfigUsbCdc& usb_cdc_config,
                   const ConsoleConfig& console_config)
{
    if (initialized_)
    {
        logger_.Warning("already initialized");
        return false;
    }
    logger_.Info("Init(USB-CDC)");
    if (!InitPeripheral(usb_cdc_config))
    {
        return false;
    }
    bool ok = FinalizeInit(console_config);
    if (ok)
        logger_.Info("Init done (backend=USB-CDC)");
    return ok;
}

bool Console::Init(const ConsolePeripheralConfigUsbSerialJtag& usb_serial_jtag_config,
                   const ConsoleConfig& console_config)
{
    if (initialized_)
    {
        logger_.Warning("already initialized");
        return false;
    }
    logger_.Info("Init(USB-Serial-JTAG, tx_buf=%lu, rx_buf=%lu)",
                 static_cast<unsigned long>(usb_serial_jtag_config.tx_buffer_size),
                 static_cast<unsigned long>(usb_serial_jtag_config.rx_buffer_size));
    if (!InitPeripheral(usb_serial_jtag_config))
    {
        return false;
    }
    bool ok = FinalizeInit(console_config);
    if (ok)
        logger_.Info("Init done (backend=USB-Serial-JTAG)");
    return ok;
}

bool Console::Deinit()
{
    if (!initialized_)
    {
        return true;
    }
    logger_.Info("Deinit");
    if (is_started_)
    {
        if (!Stop())
        {
            logger_.Error("Deinit refused: console task still running after Stop() timeout");
            return false;
        }
    }
    DeinitConsole();
    DeinitPeripheral();
    prompt_[0] = '\0';
    history_path_ = nullptr;
    initialized_ = false;
    logger_.Info("Deinit done");
    return true;
}

bool Console::RegisterCommand(const ConsoleCommand& command)
{
    if (!initialized_)
    {
        logger_.Error("RegisterCommand: not initialized");
        return false;
    }
    if (command.command == nullptr || command.command[0] == '\0')
    {
        logger_.Error("RegisterCommand: command name is empty");
        return false;
    }
    if (command.func == nullptr && command.func_w_context == nullptr)
    {
        logger_.Error("RegisterCommand('%s'): no callback provided", command.command);
        return false;
    }

    esp_err_t err = esp_console_cmd_register(static_cast<const esp_console_cmd_t*>(&command));
    if (err != ESP_OK)
    {
        logger_.Error("esp_console_cmd_register('%s') failed: %s", command.command,
                      esp_err_to_name(err));
        return false;
    }
    logger_.Info("RegisterCommand('%s')", command.command);
    return true;
}

bool Console::RegisterHelpCommand()
{
    if (!initialized_)
    {
        logger_.Error("RegisterHelpCommand: not initialized");
        return false;
    }
    esp_err_t err = esp_console_register_help_command();
    if (err != ESP_OK)
    {
        logger_.Error("esp_console_register_help_command failed: %s", esp_err_to_name(err));
        return false;
    }
    logger_.Info("RegisterHelpCommand done");
    return true;
}

bool Console::Start()
{
    if (!initialized_)
    {
        logger_.Error("Start: not initialized");
        return false;
    }
    if (is_started_)
    {
        logger_.Warning("Start: already started");
        return false;
    }
    should_exit_.store(false);
    TaskHandle_t handle = nullptr;
    BaseType_t rc =
        xTaskCreate(&Console::ConsoleLoop, "console", kTaskStackSize, this, kTaskPriority, &handle);
    if (rc != pdPASS)
    {
        logger_.Error("xTaskCreate('console') failed: %ld", static_cast<long>(rc));
        return false;
    }
    task_handle_.store(handle);
    is_started_ = true;
    logger_.Info("Start: task created (stack=%lu, prio=%u)",
                 static_cast<unsigned long>(kTaskStackSize), static_cast<unsigned>(kTaskPriority));
    return true;
}

bool Console::Stop()
{
    if (!is_started_)
    {
        return true;
    }
    logger_.Info("Stop: signaling console task to exit");
    should_exit_.store(true);

    const uint32_t poll_ticks = pdMS_TO_TICKS(kStopPollIntervalMs);
    uint32_t waited_ms = 0;
    while (task_handle_.load() != nullptr && waited_ms < kStopTimeoutMs)
    {
        vTaskDelay(poll_ticks);
        waited_ms += kStopPollIntervalMs;
    }
    if (task_handle_.load() != nullptr)
    {
        logger_.Warning(
            "Stop: console task did not exit within %lums (blocked in linenoise read); "
            "is_started_ remains true",
            static_cast<unsigned long>(kStopTimeoutMs));
        return false;
    }
    is_started_ = false;
    logger_.Info("Stop done");
    return true;
}

void Console::ConsoleLoop(void* arg)
{
    Console* self = static_cast<Console*>(arg);
    self->logger_.Info("console task started");

    while (!self->should_exit_.load())
    {
        char* line = linenoise(self->prompt_);
        if (self->should_exit_.load())
        {
            if (line != nullptr)
            {
                linenoiseFree(line);
            }
            break;
        }
        if (line == nullptr)
        {
            continue;
        }

        if (std::strlen(line) > 0)
        {
            linenoiseHistoryAdd(line);
            if (self->history_path_ != nullptr)
            {
                linenoiseHistorySave(self->history_path_);
            }
        }

        int ret = 0;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND)
        {
            std::printf("Unrecognized command\n");
        }
        else if (err == ESP_ERR_INVALID_ARG)
        {
            // empty command
        }
        else if (err == ESP_OK && ret != ESP_OK)
        {
            std::printf("Command returned non-zero error code: 0x%x (%s)\n", ret,
                        esp_err_to_name(ret));
        }
        else if (err != ESP_OK)
        {
            std::printf("Internal error: %s\n", esp_err_to_name(err));
        }

        linenoiseFree(line);
    }

    self->logger_.Info("console task exiting");
    self->task_handle_.store(nullptr);
    vTaskDelete(nullptr);
}

}  // namespace wrapper
