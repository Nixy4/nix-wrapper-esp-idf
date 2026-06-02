#include "m5stack_comx_lte.hpp"

namespace wrapper
{

M5StackComXLTE::M5StackComXLTE(Logger& logger) : AtDevice(logger) {}

M5StackComXLTE::~M5StackComXLTE() {}

bool M5StackComXLTE::Init(UartPort& port) { return AtDevice::Init(port); }

bool M5StackComXLTE::Deinit() { return AtDevice::Deinit(); }

// --- private helper ---

bool M5StackComXLTE::SendAndExpectOk(const char* cmd, std::string& response, int timeout_ms)
{
    FlushInput();
    if (WriteAtCmd(cmd) < 0)
    {
        return false;
    }
    return WaitForKeyword("OK", response, timeout_ms);
}

// --- AT: Basic test ---

bool M5StackComXLTE::Test(int timeout_ms)
{
    std::string response;
    return SendAndExpectOk("AT", response, timeout_ms);
}

// --- A/: Re-issue last command ---
// Sends "A/" without CR/LF; the module re-executes the previous command.
// Maximum response time follows the previous command (up to 120000ms).

bool M5StackComXLTE::RepeatLastCommand(int timeout_ms)
{
    FlushInput();
    int written = WriteBytes(reinterpret_cast<const uint8_t*>("A/\r"), 3);
    if (written < 0)
    {
        return false;
    }
    std::string response;
    return WaitForKeyword("OK", response, timeout_ms);
}

// --- ATD: Mobile Originated Call to Dial A Number ---
// Responses: OK / NO CARRIER / NO ANSWER / NO DIALTONE / BUSY / CONNECT<text>
// <mgsm>: I=activate CLIR, i=deactivate CLIR, G=activate CUG, g=deactivate CUG

bool M5StackComXLTE::DialNumber(const std::string& number,
                                const std::string& mgsm,
                                int timeout_ms,
                                int connect_timeout_ms)
{
    std::string cmd = "ATD" + number + mgsm + ";";
    FlushInput();
    if (WriteAtCmd(cmd.c_str()) < 0)
    {
        logger_.Error("DialNumber: 发送拨号指令失败");
        return false;
    }

    // --- 第一阶段：等待指令被接受或立即失败 ---
    std::string response;
    if (!WaitForAnyKeyword({"OK", "CONNECT", "VOICE CALL:BEGIN", "NO CARRIER", "NO ANSWER",
                            "NO DIALTONE", "BUSY", "+CME ERROR"},
                           response, timeout_ms))
    {
        logger_.Warning("DialNumber: 等待响应超时，号码: %s", number.c_str());
        return false;
    }

    // 立即失败类响应
    if (response.find("+CME ERROR") != std::string::npos)
    {
        logger_.Error("DialNumber: ME 功能错误 -> %s", response.c_str());
        return false;
    }
    if (response.find("NO DIALTONE") != std::string::npos)
    {
        logger_.Warning("DialNumber: 无拨号音，请检查线路");
        return false;
    }
    if (response.find("BUSY") != std::string::npos)
    {
        logger_.Warning("DialNumber: 对端忙线，号码: %s", number.c_str());
        return false;
    }
    if (response.find("NO ANSWER") != std::string::npos)
    {
        logger_.Warning("DialNumber: 对端无应答，号码: %s", number.c_str());
        return false;
    }
    if (response.find("NO CARRIER") != std::string::npos)
    {
        logger_.Warning("DialNumber: 无法建立连接（NO CARRIER），号码: %s", number.c_str());
        return false;
    }

    // 数据呼叫：CONNECT 直接表示成功
    if (response.find("CONNECT") != std::string::npos)
    {
        logger_.Info("DialNumber: 数据连接已建立 -> %s", response.c_str());
        return true;
    }

    // 第一阶段已包含 VOICE CALL:BEGIN，无需第二阶段
    if (response.find("VOICE CALL:BEGIN") != std::string::npos)
    {
        logger_.Info("DialNumber: 语音呼叫已接通，号码: %s", number.c_str());
        return true;
    }

    // --- 第二阶段：收到 OK 表示呼叫已发出，继续等待接通或失败 ---
    // 使用独立的 connect_timeout_ms，默认 60s 对齐 ATS7
    if (response.find("OK") != std::string::npos)
    {
        logger_.Info("DialNumber: 呼叫已发出，等待接通（最长 %dms），号码: %s", connect_timeout_ms,
                     number.c_str());
        std::string response2;
        if (!WaitForAnyKeyword({"VOICE CALL:BEGIN", "CONNECT", "+RXDTMF", "NO CARRIER", "NO ANSWER",
                                "BUSY", "+CME ERROR"},
                               response2, connect_timeout_ms))
        {
            logger_.Warning("DialNumber: 等待接通超时，号码: %s", number.c_str());
            return false;
        }
        if (response2.find("VOICE CALL:BEGIN") != std::string::npos)
        {
            logger_.Info("DialNumber: 语音呼叫已接通，号码: %s", number.c_str());
            return true;
        }
        if (response2.find("+RXDTMF") != std::string::npos)
        {
            logger_.Info("DialNumber: 检测到 DTMF 信号（通话已建立），号码: %s", number.c_str());
            return true;
        }
        if (response2.find("CONNECT") != std::string::npos)
        {
            logger_.Info("DialNumber: 数据连接已建立 -> %s", response2.c_str());
            return true;
        }
        if (response2.find("NO CARRIER") != std::string::npos)
            logger_.Warning("DialNumber: 呼叫未接通（NO CARRIER），号码: %s", number.c_str());
        else if (response2.find("NO ANSWER") != std::string::npos)
            logger_.Warning("DialNumber: 对端无应答，号码: %s", number.c_str());
        else if (response2.find("BUSY") != std::string::npos)
            logger_.Warning("DialNumber: 对端忙线，号码: %s", number.c_str());
        else if (response2.find("+CME ERROR") != std::string::npos)
            logger_.Error("DialNumber: ME 功能错误 -> %s", response2.c_str());
        else
            logger_.Warning("DialNumber: 未知响应 -> %s", response2.c_str());
        return false;
    }

    logger_.Warning("DialNumber: 未知响应 -> %s", response.c_str());
    return false;
}

// --- ATD><mem><n>: Originate call from specified memory ---
// <mem>: "DC"/"MC"/"RC"/"SM"/"ME"/"FD"/"ON"/"LD"/"EN"
// <n>: index returned by AT+CPBR
// voice_call=true appends ";", must NOT be used for data/fax calls

bool M5StackComXLTE::DialSpecifiedMem(const std::string& mem,
                                      int index,
                                      bool voice_call,
                                      int timeout_ms)
{
    std::string cmd = "ATD>" + mem + std::to_string(index);
    if (voice_call)
    {
        cmd += ";";
    }
    FlushInput();
    if (WriteAtCmd(cmd.c_str()) < 0)
    {
        logger_.Error("DialMem: 发送指令失败，存储器: %s 索引: %d", mem.c_str(), index);
        return false;
    }
    std::string response;
    if (!WaitForAnyKeyword(
            {"OK", "CONNECT", "VOICE CALL:BEGIN", "NO CARRIER", "ERROR", "+CME ERROR"}, response,
            timeout_ms))
    {
        logger_.Warning("DialMem: 等待响应超时，存储器: %s 索引: %d", mem.c_str(), index);
        return false;
    }
    if (response.find("+CME ERROR") != std::string::npos)
    {
        logger_.Error("DialMem: MT 错误 -> %s", response.c_str());
        return false;
    }
    if (response.find("NO CARRIER") != std::string::npos)
    {
        logger_.Warning("DialMem: 连接恢复失败（NO CARRIER），存储器: %s 索引: %d", mem.c_str(),
                        index);
        return false;
    }
    if (response.find("ERROR") != std::string::npos)
    {
        logger_.Error("DialMem: 指令执行失败，存储器: %s 索引: %d", mem.c_str(), index);
        return false;
    }
    if (response.find("CONNECT") != std::string::npos)
    {
        logger_.Info("DialMem: 数据连接已建立 -> %s", response.c_str());
        return true;
    }
    if (response.find("VOICE CALL:BEGIN") != std::string::npos ||
        response.find("OK") != std::string::npos)
    {
        logger_.Info("DialMem: 语音呼叫已发起，存储器: %s 索引: %d", mem.c_str(), index);
        return true;
    }
    logger_.Warning("DialMem: 未知响应 -> %s", response.c_str());
    return false;
}

// --- ATD><n>: Originate call from active memory ---
// Uses the currently active phonebook (set by AT+CPBS); <n> = index from AT+CPBR.
// voice_call=true appends ";"; must NOT be used for data/fax calls.
// Note: not supported by some telecom operators.

bool M5StackComXLTE::DialActiveMem(int index, bool voice_call, int timeout_ms)
{
    std::string cmd = "ATD>" + std::to_string(index);
    if (voice_call)
    {
        cmd += ";";
    }
    FlushInput();
    if (WriteAtCmd(cmd.c_str()) < 0)
    {
        logger_.Error("DialActiveMem: 发送指令失败，索引: %d", index);
        return false;
    }
    std::string response;
    if (!WaitForAnyKeyword(
            {"OK", "CONNECT", "VOICE CALL:BEGIN", "NO CARRIER", "ERROR", "+CME ERROR"}, response,
            timeout_ms))
    {
        logger_.Warning("DialActiveMem: 等待响应超时，索引: %d", index);
        return false;
    }
    if (response.find("+CME ERROR") != std::string::npos)
    {
        logger_.Error("DialActiveMem: MT 错误 -> %s", response.c_str());
        return false;
    }
    if (response.find("NO CARRIER") != std::string::npos)
    {
        logger_.Warning("DialActiveMem: 连接恢复失败（NO CARRIER），索引: %d", index);
        return false;
    }
    if (response.find("ERROR") != std::string::npos)
    {
        logger_.Error("DialActiveMem: 指令执行失败，索引: %d", index);
        return false;
    }
    if (response.find("CONNECT") != std::string::npos)
    {
        logger_.Info("DialActiveMem: 数据连接已建立 -> %s", response.c_str());
        return true;
    }
    if (response.find("VOICE CALL:BEGIN") != std::string::npos ||
        response.find("OK") != std::string::npos)
    {
        logger_.Info("DialActiveMem: 语音呼叫已发起，索引: %d", index);
        return true;
    }
    logger_.Warning("DialActiveMem: 未知响应 -> %s", response.c_str());
    return false;
}

// --- ATD><str>: Originate call from active memory by name ---
// <str> must be double-quoted and match an alphanumeric field in the phonebook.
// Encoding follows AT+CSCS current TE character set.
// voice_call=true appends ";"; must NOT be used for data/fax calls.
// Note: not supported by some telecom operators.

bool M5StackComXLTE::DialActiveMemByName(const std::string& name, bool voice_call, int timeout_ms)
{
    // Wrap name in double quotes as required by the spec
    std::string cmd = "ATD>\"" + name + "\"";
    if (voice_call)
    {
        cmd += ";";
    }
    FlushInput();
    if (WriteAtCmd(cmd.c_str()) < 0)
    {
        logger_.Error("DialActiveMemByName: 发送指令失败，名称: %s", name.c_str());
        return false;
    }
    std::string response;
    if (!WaitForAnyKeyword(
            {"OK", "CONNECT", "VOICE CALL:BEGIN", "NO CARRIER", "ERROR", "+CME ERROR"}, response,
            timeout_ms))
    {
        logger_.Warning("DialActiveMemByName: 等待响应超时，名称: %s", name.c_str());
        return false;
    }
    if (response.find("+CME ERROR") != std::string::npos)
    {
        logger_.Error("DialActiveMemByName: MT 错误 -> %s", response.c_str());
        return false;
    }
    if (response.find("NO CARRIER") != std::string::npos)
    {
        logger_.Warning("DialActiveMemByName: 连接恢复失败（NO CARRIER），名称: %s", name.c_str());
        return false;
    }
    if (response.find("ERROR") != std::string::npos)
    {
        logger_.Error("DialActiveMemByName: 指令执行失败，名称: %s", name.c_str());
        return false;
    }
    if (response.find("CONNECT") != std::string::npos)
    {
        logger_.Info("DialActiveMemByName: 数据连接已建立 -> %s", response.c_str());
        return true;
    }
    if (response.find("VOICE CALL:BEGIN") != std::string::npos ||
        response.find("OK") != std::string::npos)
    {
        logger_.Info("DialActiveMemByName: 语音呼叫已发起，名称: %s", name.c_str());
        return true;
    }
    logger_.Warning("DialActiveMemByName: 未知响应 -> %s", response.c_str());
    return false;
}

// --- ATA: Answer incoming call ---
// If no incoming call, TA returns NO CARRIER.

bool M5StackComXLTE::AnswerCall(int timeout_ms)
{
    FlushInput();
    if (WriteAtCmd("ATA") < 0)
    {
        logger_.Error("AnswerCall: 发送指令失败");
        return false;
    }
    std::string response;
    if (!WaitForAnyKeyword({"OK", "CONNECT", "VOICE CALL:BEGIN", "NO CARRIER"}, response,
                           timeout_ms))
    {
        logger_.Warning("AnswerCall: 等待响应超时");
        return false;
    }
    if (response.find("NO CARRIER") != std::string::npos)
    {
        logger_.Warning("AnswerCall: 无连接或无来电（NO CARRIER）");
        return false;
    }
    if (response.find("CONNECT") != std::string::npos)
    {
        logger_.Info("AnswerCall: 数据呼叫已接听，TA 进入数据模式");
        return true;
    }
    logger_.Info("AnswerCall: 语音呼叫已接听");
    return true;
}

// --- ATE: Echo mode ---

bool M5StackComXLTE::SetEchoMode(bool enable, int timeout_ms)
{
    std::string response;
    return SendAndExpectOk(enable ? "ATE1" : "ATE0", response, timeout_ms);
}

// --- ATH: Hangup ---

bool M5StackComXLTE::Hangup(int timeout_ms)
{
    std::string response;
    return SendAndExpectOk("ATH", response, timeout_ms);
}

// --- ATI: Product info ---

bool M5StackComXLTE::GetProductInfo(std::string& info, int timeout_ms)
{
    return SendAndExpectOk("ATI", info, timeout_ms);
}

// --- +++: Switch to command mode ---

bool M5StackComXLTE::SwitchToCommandMode(int timeout_ms)
{
    // +++ requires a guard time silence before and after (typically 1s)
    vTaskDelay(pdMS_TO_TICKS(1100));
    FlushInput();
    // +++ is sent without CR/LF
    int written = WriteBytes(reinterpret_cast<const uint8_t*>("+++"), 3);
    if (written < 0)
    {
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(1100));
    std::string response;
    return WaitForKeyword("OK", response, timeout_ms);
}

// --- ATO: Switch to data mode ---

bool M5StackComXLTE::SwitchToDataMode(int timeout_ms)
{
    std::string response;
    return SendAndExpectOk("ATO", response, timeout_ms);
}

// --- ATS0: Auto answer ---

bool M5StackComXLTE::SetAutoAnswer(int rings, int timeout_ms)
{
    std::string cmd = "ATS0=" + std::to_string(rings);
    std::string response;
    return SendAndExpectOk(cmd.c_str(), response, timeout_ms);
}

// --- ATZ: Reset default configuration ---

bool M5StackComXLTE::ResetDefaultConfiguration(int timeout_ms)
{
    std::string response;
    return SendAndExpectOk("ATZ", response, timeout_ms);
}

// --- AT+GCAP: Capabilities ---

bool M5StackComXLTE::GetCapabilities(std::string& caps, int timeout_ms)
{
    return SendAndExpectOk("AT+GCAP", caps, timeout_ms);
}

// --- AT+GMI: Manufacturer ---

bool M5StackComXLTE::GetManufacturer(std::string& info, int timeout_ms)
{
    return SendAndExpectOk("AT+GMI", info, timeout_ms);
}

// --- AT+GMM: Model ---

bool M5StackComXLTE::GetModel(std::string& info, int timeout_ms)
{
    return SendAndExpectOk("AT+GMM", info, timeout_ms);
}

// --- AT+GMR: Revision ---

bool M5StackComXLTE::GetRevision(std::string& info, int timeout_ms)
{
    return SendAndExpectOk("AT+GMR", info, timeout_ms);
}

// --- AT+GSN: IMEI ---

bool M5StackComXLTE::GetIMEI(std::string& imei, int timeout_ms)
{
    return SendAndExpectOk("AT+GSN", imei, timeout_ms);
}

// --- AT+IPR: Fixed baud rate ---

bool M5StackComXLTE::SetFixedBaudRate(int rate, int timeout_ms)
{
    std::string cmd = "AT+IPR=" + std::to_string(rate);
    std::string response;
    return SendAndExpectOk(cmd.c_str(), response, timeout_ms);
}

}  // namespace wrapper
