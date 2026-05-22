#pragma once
#include <cstring>
#include <string>
#include <vector>
#include "driver/uart.h"
#include "wrapper/logger.hpp"

namespace wrapper
{

struct UartConfig : public uart_config_t
{
   public:
    UartConfig(int baud_rate, uart_word_length_t data_bits, uart_parity_t parity,
               uart_stop_bits_t stop_bits, uart_hw_flowcontrol_t flow_ctrl,
               uint8_t rx_flow_ctrl_thresh, uart_sclk_t source_clk)
        : uart_config_t{}
    {
        this->baud_rate = baud_rate;
        this->data_bits = data_bits;
        this->parity = parity;
        this->stop_bits = stop_bits;
        this->flow_ctrl = flow_ctrl;
        this->rx_flow_ctrl_thresh = rx_flow_ctrl_thresh;
        this->source_clk = source_clk;
    }
};

class UartPort
{
    Logger& logger_;
    uart_port_t port_;
    bool installed_;
    QueueHandle_t event_queue_;

   public:
    UartPort(Logger& logger);
    ~UartPort();

    Logger& GetLogger();
    uart_port_t GetPort() const;
    QueueHandle_t GetEventQueue() const;
    bool IsInstalled() const;

    // operations
    bool Init(uart_port_t port, const UartConfig& config, int tx_pin, int rx_pin,
              int rts_pin = UART_PIN_NO_CHANGE, int cts_pin = UART_PIN_NO_CHANGE,
              int rx_buffer_size = 1024, int tx_buffer_size = 0, int event_queue_size = 0);
    bool Deinit();

    // baudrate
    bool SetBaudrate(uint32_t baudrate);
    bool GetBaudrate(uint32_t& baudrate);
};

class UartDevice
{
   protected:
    Logger& logger_;
    UartPort* port_;

   public:
    UartDevice(Logger& logger);
    ~UartDevice();
    Logger& GetLogger();
    bool Init(UartPort& port);
    bool Deinit();

    // --- write: raw pointer (inline) ---

    inline int WriteBytes(const uint8_t* data, size_t len)
    {
        return uart_write_bytes(port_->GetPort(), data, len);
    }

    inline int WriteByte(uint8_t data) { return uart_write_bytes(port_->GetPort(), &data, 1); }

    // --- write: vector / string ---

    int WriteBytes(const std::vector<uint8_t>& data);

    inline int Write(const char* str)
    {
        return uart_write_bytes(port_->GetPort(), str, strlen(str));
    }

    inline int Write(const std::string& str)
    {
        return uart_write_bytes(port_->GetPort(), str.c_str(), str.size());
    }

    int WriteLine(const std::string& line, char delimiter = '\n');

    // --- read: raw pointer (inline) ---

    inline int ReadBytes(uint8_t* buf, size_t len, int timeout_ms)
    {
        return uart_read_bytes(port_->GetPort(), buf, len, pdMS_TO_TICKS(timeout_ms));
    }

    // --- read: single byte / vector ---

    int ReadByte(uint8_t& data, int timeout_ms);

    int ReadBytes(std::vector<uint8_t>& buf, size_t len, int timeout_ms);

    int ReadAvailable(std::vector<uint8_t>& buf, int timeout_ms);

    bool ReadLine(std::string& line, char delimiter, int timeout_ms);

    // --- buffer control ---
    bool Flush();
    bool FlushInput();
    bool WaitTxDone(int timeout_ms);
    int GetBufferedDataLen();
};

class AtDevice : public UartDevice
{
   public:
    AtDevice(Logger& logger);

    int WriteAtCmd(const char* cmd);
    int WriteAtCmd(const std::string& cmd);

    bool SendAtCmd(const char* cmd, std::string& response, int timeout_ms = 3000);
    bool SendAtCmd(const std::string& cmd, std::string& response, int timeout_ms = 3000);

    bool WaitForKeyword(const std::string& keyword, std::string& response, int timeout_ms = 3000);
};

}  // namespace wrapper
