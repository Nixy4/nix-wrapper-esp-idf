#include "m5stack_comx_lte.hpp"

namespace wrapper
{

M5StackComXLTE::M5StackComXLTE(Logger &logger) : AtDevice(logger)
{
}

M5StackComXLTE::~M5StackComXLTE()
{
}

bool M5StackComXLTE::Init(UartPort &port)
{
    return AtDevice::Init(port);
}

bool M5StackComXLTE::Deinit()
{
    return AtDevice::Deinit();
}

// --- private helper ---

bool M5StackComXLTE::SendAndExpectOk(const char *cmd, std::string &response, int timeout_ms)
{
    FlushInput();
    if (WriteAtCmd(cmd) < 0) {
        return false;
    }
    return WaitForKeyword("OK", response, timeout_ms);
}

// --- Basic control ---

bool M5StackComXLTE::Test(int timeout_ms)
{
    std::string response;
    return SendAndExpectOk("AT", response, timeout_ms);
}

bool M5StackComXLTE::Reset(int timeout_ms)
{
    std::string response;
    return SendAndExpectOk("ATZ", response, timeout_ms);
}

bool M5StackComXLTE::SetEchoMode(bool enable, int timeout_ms)
{
    std::string response;
    return SendAndExpectOk(enable ? "ATE1" : "ATE0", response, timeout_ms);
}

// --- Device information ---

bool M5StackComXLTE::GetProductInfo(std::string &info, int timeout_ms)
{
    return SendAndExpectOk("ATI", info, timeout_ms);
}

bool M5StackComXLTE::GetManufacturer(std::string &info, int timeout_ms)
{
    return SendAndExpectOk("AT+GMI", info, timeout_ms);
}

bool M5StackComXLTE::GetModel(std::string &info, int timeout_ms)
{
    return SendAndExpectOk("AT+GMM", info, timeout_ms);
}

bool M5StackComXLTE::GetRevision(std::string &info, int timeout_ms)
{
    return SendAndExpectOk("AT+GMR", info, timeout_ms);
}

bool M5StackComXLTE::GetIMEI(std::string &imei, int timeout_ms)
{
    return SendAndExpectOk("AT+GSN", imei, timeout_ms);
}

bool M5StackComXLTE::GetCapabilities(std::string &caps, int timeout_ms)
{
    return SendAndExpectOk("AT+GCAP", caps, timeout_ms);
}

// --- Call control ---

bool M5StackComXLTE::Dial(const std::string &number, int timeout_ms)
{
    std::string cmd = "ATD" + number + ";";
    std::string response;
    return SendAndExpectOk(cmd.c_str(), response, timeout_ms);
}

bool M5StackComXLTE::Hangup(int timeout_ms)
{
    std::string response;
    return SendAndExpectOk("ATH", response, timeout_ms);
}

bool M5StackComXLTE::SetAutoAnswer(int rings, int timeout_ms)
{
    std::string cmd = "ATS0=" + std::to_string(rings);
    std::string response;
    return SendAndExpectOk(cmd.c_str(), response, timeout_ms);
}

// --- Mode switching ---

bool M5StackComXLTE::SwitchToCommandMode(int timeout_ms)
{
    // +++ requires a guard time silence before and after (typically 1s)
    vTaskDelay(pdMS_TO_TICKS(1100));
    FlushInput();
    // +++ is sent without CR/LF
    int written = WriteBytes(reinterpret_cast<const uint8_t *>("+++"), 3);
    if (written < 0) {
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(1100));
    std::string response;
    return WaitForKeyword("OK", response, timeout_ms);
}

bool M5StackComXLTE::SwitchToDataMode(int timeout_ms)
{
    std::string response;
    return SendAndExpectOk("ATO", response, timeout_ms);
}

// --- Configuration ---

bool M5StackComXLTE::SetFixedBaudRate(int rate, int timeout_ms)
{
    std::string cmd = "AT+IPR=" + std::to_string(rate);
    std::string response;
    return SendAndExpectOk(cmd.c_str(), response, timeout_ms);
}

} // namespace wrapper
