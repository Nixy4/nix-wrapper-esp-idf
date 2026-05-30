#pragma once

#include <atomic>

#include "esp_console.h"
#include "esp_log.h"

#include "driver/uart.h"
#include "driver/uart_vfs.h"
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "wrapper/logger.hpp"

namespace wrapper
{

struct ConsolePeripheralConfigUart : public uart_config_t
{
    uart_port_t uart_num;
    int rx_buffer_size;

    ConsolePeripheralConfigUart(uart_port_t uart_num = UART_NUM_0,
                                int baud_rate = 115200,
                                uart_word_length_t data_bits = UART_DATA_8_BITS,
                                uart_parity_t parity = UART_PARITY_DISABLE,
                                uart_stop_bits_t stop_bits = UART_STOP_BITS_1,
                                uart_hw_flowcontrol_t flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                                uint8_t rx_flow_ctrl_thresh = 0,
                                uart_sclk_t source_clk = UART_SCLK_DEFAULT,
                                bool allow_pd = false,
                                bool backup_before_sleep = false,
                                int rx_buffer_size = 128)
        : uart_config_t{}
    {
        this->uart_num = uart_num;
        this->rx_buffer_size = rx_buffer_size;
        this->baud_rate = baud_rate;
        this->data_bits = data_bits;
        this->parity = parity;
        this->stop_bits = stop_bits;
        this->flow_ctrl = flow_ctrl;
        this->rx_flow_ctrl_thresh = rx_flow_ctrl_thresh;
        this->source_clk = source_clk;
        this->flags.allow_pd = allow_pd ? 1 : 0;
        this->flags.backup_before_sleep = backup_before_sleep ? 1 : 0;
    }
};

struct ConsolePeripheralConfigUsbCdc
{
    esp_line_endings_t tx_line_endings;
    esp_line_endings_t rx_line_endings;

    ConsolePeripheralConfigUsbCdc(esp_line_endings_t tx_line_endings = ESP_LINE_ENDINGS_CRLF,
                                  esp_line_endings_t rx_line_endings = ESP_LINE_ENDINGS_CR)
        : tx_line_endings(tx_line_endings), rx_line_endings(rx_line_endings)
    {
    }
};

struct ConsolePeripheralConfigUsbSerialJtag : public usb_serial_jtag_driver_config_t
{
    esp_line_endings_t tx_line_endings;
    esp_line_endings_t rx_line_endings;

    ConsolePeripheralConfigUsbSerialJtag(uint32_t tx_buffer_size = 128,
                                         uint32_t rx_buffer_size = 128,
                                         esp_line_endings_t tx_line_endings = ESP_LINE_ENDINGS_CRLF,
                                         esp_line_endings_t rx_line_endings = ESP_LINE_ENDINGS_CR)
        : usb_serial_jtag_driver_config_t{},
          tx_line_endings(tx_line_endings),
          rx_line_endings(rx_line_endings)
    {
        this->tx_buffer_size = tx_buffer_size;
        this->rx_buffer_size = rx_buffer_size;
    }
};

struct ConsoleConfig : public esp_console_config_t
{
    // Lifetime of these C-strings must outlive Console::Init().
    // Pass string literals or otherwise long-lived buffers.
    const char* prompt;
    const char* history_save_path;
    size_t history_max_len;

    ConsoleConfig(size_t max_cmdline_length = 256,
                  size_t max_cmdline_args = 16,
                  uint32_t heap_alloc_caps = MALLOC_CAP_DEFAULT,
#if CONFIG_LOG_COLORS
                  int hint_color = 36,  // cyan
                  int hint_bold = 0,
#else
                  int hint_color = 0,
                  int hint_bold = 0,
#endif
                  const char* prompt = nullptr,
                  const char* history_save_path = nullptr,
                  size_t history_max_len = 20)
        : esp_console_config_t{},
          prompt(prompt),
          history_save_path(history_save_path),
          history_max_len(history_max_len)
    {
        this->max_cmdline_length = max_cmdline_length;
        this->max_cmdline_args = max_cmdline_args;
        this->heap_alloc_caps = heap_alloc_caps;
        this->hint_color = hint_color;
        this->hint_bold = hint_bold;
    }
};

struct ConsoleCommand : public esp_console_cmd_t
{
    ConsoleCommand(const char* command,
                   const char* help,
                   const char* hint,
                   esp_console_cmd_func_t func,
                   void* argtable = nullptr,
                   esp_console_cmd_func_with_context_t func_w_context = nullptr,
                   void* context = nullptr)
        : esp_console_cmd_t{}
    {
        this->command = command;
        this->help = help;
        this->hint = hint;
        this->func = func;
        this->argtable = argtable;
        this->func_w_context = func_w_context;
        this->context = context;
    }
};

class Console
{
   public:
    enum class Backend
    {
        None,
        Uart,
        UsbCdc,
        UsbSerialJtag,
    };

    Console();
    ~Console();

    Console(const Console&) = delete;
    Console& operator=(const Console&) = delete;

    bool Init(const ConsolePeripheralConfigUart& uart_config, const ConsoleConfig& console_config);
    bool Init(const ConsolePeripheralConfigUsbCdc& usb_cdc_config,
              const ConsoleConfig& console_config);
    bool Init(const ConsolePeripheralConfigUsbSerialJtag& usb_serial_jtag_config,
              const ConsoleConfig& console_config);
    bool Deinit();

    // esp_console_cmd_register internally strdup's command/help/hint, so the caller
    // does not need to keep `command` alive after the call returns.
    bool RegisterCommand(const ConsoleCommand& command);

    bool RegisterHelpCommand();

    bool Start();
    bool Stop();

    Backend GetBackend() const { return backend_; }
    bool IsInitialized() const { return initialized_; }
    bool IsStarted() const { return is_started_; }

   private:
    static constexpr size_t kPromptBufferSize = 64;

    Logger logger_{"console"};
    bool initialized_ = false;
    bool is_started_ = false;
    Backend backend_ = Backend::None;

    uart_port_t uart_port_ = UART_NUM_0;

    char prompt_[kPromptBufferSize] = {};
    const char* history_path_ = nullptr;  // borrowed; caller-owned lifetime

    std::atomic<TaskHandle_t> task_handle_{nullptr};
    std::atomic<bool> should_exit_{false};

    bool InitPeripheral(const ConsolePeripheralConfigUart& uart_config);
    bool InitPeripheral(const ConsolePeripheralConfigUsbCdc& usb_cdc_config);
    bool InitPeripheral(const ConsolePeripheralConfigUsbSerialJtag& usb_serial_jtag_config);
    bool DeinitPeripheral();

    bool InitConsole(const ConsoleConfig& console_config);
    bool DeinitConsole();

    void SetupPrompt(const char* prompt);

    bool FinalizeInit(const ConsoleConfig& console_config);

    static void ConsoleLoop(void* arg);
};

}  // namespace wrapper
