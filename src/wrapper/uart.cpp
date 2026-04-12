#include "wrapper/uart.hpp"

using namespace wrapper;

// --- UartPort ---

UartPort::UartPort(Logger &logger) : logger_(logger), port_(UART_NUM_0), installed_(false), event_queue_(nullptr)
{
}

UartPort::~UartPort()
{
    Deinit();
}

Logger &UartPort::GetLogger()
{
    return logger_;
}

uart_port_t UartPort::GetPort() const
{
    return port_;
}

QueueHandle_t UartPort::GetEventQueue() const
{
    return event_queue_;
}

bool UartPort::IsInstalled() const
{
    return installed_;
}

bool UartPort::Init(uart_port_t port,
                    const UartConfig &config,
                    int tx_pin,
                    int rx_pin,
                    int rts_pin,
                    int cts_pin,
                    int rx_buffer_size,
                    int tx_buffer_size,
                    int event_queue_size)
{
    if (installed_) {
        logger_.Warning("Already initialized. Deinitializing first.");
        Deinit();
    }

    port_ = port;

    esp_err_t ret = uart_param_config(port_, &config);
    if (ret != ESP_OK) {
        logger_.Error("Failed to configure parameters: %s", esp_err_to_name(ret));
        return false;
    }

    ret = uart_set_pin(port_, tx_pin, rx_pin, rts_pin, cts_pin);
    if (ret != ESP_OK) {
        logger_.Error("Failed to set pins: %s", esp_err_to_name(ret));
        return false;
    }

    QueueHandle_t *queue_ptr = (event_queue_size > 0) ? &event_queue_ : nullptr;
    ret = uart_driver_install(port_, rx_buffer_size, tx_buffer_size, event_queue_size, queue_ptr, 0);
    if (ret != ESP_OK) {
        logger_.Error("Failed to install driver: %s", esp_err_to_name(ret));
        return false;
    }

    installed_ = true;
    logger_.Info("Initialized (Port: %d, TX: %d, RX: %d, Baud: %d)",
                 port_, tx_pin, rx_pin, config.baud_rate);
    return true;
}

bool UartPort::Deinit()
{
    if (!installed_) {
        return true;
    }

    esp_err_t ret = uart_driver_delete(port_);
    if (ret != ESP_OK) {
        logger_.Error("Failed to delete driver: %s", esp_err_to_name(ret));
        return false;
    }

    installed_ = false;
    event_queue_ = nullptr;
    logger_.Info("Deinitialized (Port: %d)", port_);
    return true;
}

bool UartPort::SetBaudrate(uint32_t baudrate)
{
    esp_err_t ret = uart_set_baudrate(port_, baudrate);
    if (ret != ESP_OK) {
        logger_.Error("Failed to set baudrate: %s", esp_err_to_name(ret));
        return false;
    }
    logger_.Info("Baudrate set to %lu", baudrate);
    return true;
}

bool UartPort::GetBaudrate(uint32_t &baudrate)
{
    esp_err_t ret = uart_get_baudrate(port_, &baudrate);
    if (ret != ESP_OK) {
        logger_.Error("Failed to get baudrate: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

// --- UartDevice ---

UartDevice::UartDevice(Logger &logger) : logger_(logger), port_(nullptr)
{
}

UartDevice::~UartDevice()
{
    Deinit();
}

Logger &UartDevice::GetLogger()
{
    return logger_;
}

bool UartDevice::Init(UartPort &port)
{
    if (port_ != nullptr) {
        logger_.Warning("Device already initialized. Deinitializing first.");
        Deinit();
    }

    if (!port.IsInstalled()) {
        logger_.Error("Port not initialized");
        return false;
    }

    port_ = &port;
    logger_.Info("Device initialized (Port: %d)", port_->GetPort());
    return true;
}

bool UartDevice::Deinit()
{
    if (port_ == nullptr) {
        return true;
    }
    logger_.Info("Device deinitialized");
    port_ = nullptr;
    return true;
}

int UartDevice::WriteBytes(const std::vector<uint8_t> &data)
{
    return uart_write_bytes(port_->GetPort(), data.data(), data.size());
}

int UartDevice::ReadByte(uint8_t &data, int timeout_ms)
{
    return uart_read_bytes(port_->GetPort(), &data, 1, pdMS_TO_TICKS(timeout_ms));
}

int UartDevice::ReadBytes(std::vector<uint8_t> &buf, size_t len, int timeout_ms)
{
    buf.resize(len);
    int read = uart_read_bytes(port_->GetPort(), buf.data(), len, pdMS_TO_TICKS(timeout_ms));
    if (read >= 0) {
        buf.resize(read);
    } else {
        buf.clear();
    }
    return read;
}

int UartDevice::ReadAvailable(std::vector<uint8_t> &buf, int timeout_ms)
{
    uart_port_t p = port_->GetPort();
    size_t available = 0;
    esp_err_t ret = uart_get_buffered_data_len(p, &available);
    if (ret != ESP_OK) {
        logger_.Error("ReadAvailable: failed to get buffer length: %s", esp_err_to_name(ret));
        return -1;
    }
    if (available == 0) {
        buf.resize(1);
        int n = uart_read_bytes(p, buf.data(), 1, pdMS_TO_TICKS(timeout_ms));
        if (n <= 0) {
            buf.clear();
            return n;
        }
        uart_get_buffered_data_len(p, &available);
        if (available > 0) {
            buf.resize(1 + available);
            uart_read_bytes(p, buf.data() + 1, available, 0);
        }
        return static_cast<int>(buf.size());
    }
    buf.resize(available);
    int n = uart_read_bytes(p, buf.data(), available, pdMS_TO_TICKS(timeout_ms));
    if (n >= 0) {
        buf.resize(n);
    } else {
        buf.clear();
    }
    return n;
}

bool UartDevice::ReadLine(std::string &line, char delimiter, int timeout_ms)
{
    line.clear();
    uint8_t ch;
    uart_port_t p = port_->GetPort();
    while (true) {
        int n = uart_read_bytes(p, &ch, 1, pdMS_TO_TICKS(timeout_ms));
        if (n <= 0) {
            return false;
        }
        if (ch == static_cast<uint8_t>(delimiter)) {
            return true;
        }
        line.push_back(static_cast<char>(ch));
    }
}

int UartDevice::WriteLine(const std::string &line, char delimiter)
{
    uart_port_t p = port_->GetPort();
    int written = uart_write_bytes(p, line.c_str(), line.size());
    if (written < 0) {
        return written;
    }
    int d = uart_write_bytes(p, &delimiter, 1);
    if (d < 0) {
        return d;
    }
    return written + d;
}

bool UartDevice::Flush()
{
    esp_err_t ret = uart_flush(port_->GetPort());
    if (ret != ESP_OK) {
        logger_.Error("Failed to flush: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool UartDevice::FlushInput()
{
    esp_err_t ret = uart_flush_input(port_->GetPort());
    if (ret != ESP_OK) {
        logger_.Error("Failed to flush input: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool UartDevice::WaitTxDone(int timeout_ms)
{
    esp_err_t ret = uart_wait_tx_done(port_->GetPort(), pdMS_TO_TICKS(timeout_ms));
    if (ret != ESP_OK) {
        logger_.Error("Failed to wait TX done: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

int UartDevice::GetBufferedDataLen()
{
    size_t len = 0;
    esp_err_t ret = uart_get_buffered_data_len(port_->GetPort(), &len);
    if (ret != ESP_OK) {
        logger_.Error("Failed to get buffered data length: %s", esp_err_to_name(ret));
        return -1;
    }
    return static_cast<int>(len);
}

// --- AtDevice ---

AtDevice::AtDevice(Logger &logger) : UartDevice(logger)
{
}

int AtDevice::WriteAtCmd(const char *cmd)
{
    uart_port_t p = port_->GetPort();
    int written = uart_write_bytes(p, cmd, strlen(cmd));
    if (written < 0) {
        return written;
    }
    const char crlf[] = "\r\n";
    int d = uart_write_bytes(p, crlf, 2);
    if (d < 0) {
        return d;
    }
    logger_.Info("AT>>> %s", cmd);
    return written + d;
}

int AtDevice::WriteAtCmd(const std::string &cmd)
{
    return WriteAtCmd(cmd.c_str());
}

bool AtDevice::SendAtCmd(const char *cmd, std::string &response, int timeout_ms)
{
    FlushInput();

    if (WriteAtCmd(cmd) < 0) {
        logger_.Error("SendAtCmd: write failed");
        return false;
    }

    return WaitForKeyword("", response, timeout_ms);
}

bool AtDevice::SendAtCmd(const std::string &cmd, std::string &response, int timeout_ms)
{
    return SendAtCmd(cmd.c_str(), response, timeout_ms);
}

bool AtDevice::WaitForKeyword(const std::string &keyword, std::string &response, int timeout_ms)
{
    response.clear();
    std::string line;
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);

    while (xTaskGetTickCount() < deadline) {
        int remaining_ms = static_cast<int>((deadline - xTaskGetTickCount()) * portTICK_PERIOD_MS);
        if (remaining_ms <= 0) {
            break;
        }
        bool got = ReadLine(line, '\n', remaining_ms);
        if (!got && line.empty()) {
            break;
        }
        // 去除行尾 '\r'
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!line.empty()) {
            if (!response.empty()) {
                response += '\n';
            }
            response += line;
        }
        if (!keyword.empty() &&
            (response.find(keyword) != std::string::npos ||
             line.find(keyword) != std::string::npos)) {
            logger_.Info("AT<<< %s", response.c_str());
            return true;
        }
    }

    if (!response.empty()) {
        logger_.Info("AT<<< %s", response.c_str());
        return keyword.empty();
    }
    logger_.Warning("AT: no response (timeout)");
    return false;
}
