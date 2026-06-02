#include "wrapper/uart.hpp"

namespace wrapper
{
class M5StackComXLTE : public AtDevice
{
   public:
    constexpr static int kDefaultTimeoutMs = 3000;

    M5StackComXLTE(Logger& logger);
    ~M5StackComXLTE();

    bool Init(UartPort& port);
    bool Deinit();

    // AT  - Basic test
    bool Test(int timeout_ms = kDefaultTimeoutMs);
    // A/  - Re-issue last command
    bool RepeatLastCommand(int timeout_ms = kDefaultTimeoutMs);
    // ATD - Dial; mgsm: I/i=CLIR, G/g=CUG
    // ATD - Dial; mgsm: I/i=CLIR, G/g=CUG
    // timeout_ms:         等待模块接受指令（第一阶段，收到 OK）的超时
    // connect_timeout_ms: 等待对端接通（第二阶段，收到 VOICE CALL:BEGIN）的超时，默认对齐 ATS7=60s
    bool DialNumber(const std::string& number,
                    const std::string& mgsm = "",
                    int timeout_ms = kDefaultTimeoutMs,
                    int connect_timeout_ms = 60000);
    // ATD><mem><n> - Originate call from specified phonebook memory
    bool DialSpecifiedMem(const std::string& mem,
                          int index,
                          bool voice_call = true,
                          int timeout_ms = 10000);
    // ATD><n> - Originate call from active memory (index only, no mem name)
    bool DialActiveMem(int index, bool voice_call = true, int timeout_ms = 10000);
    // ATD><str> - Originate call from active memory by name string (double-quoted)
    bool DialActiveMemByName(const std::string& name,
                             bool voice_call = true,
                             int timeout_ms = 10000);
    // ATA - Answer incoming call
    bool AnswerCall(int timeout_ms = kDefaultTimeoutMs);
    // ATE - Echo mode
    bool SetEchoMode(bool enable, int timeout_ms = kDefaultTimeoutMs);
    // ATH - Hangup
    bool Hangup(int timeout_ms = kDefaultTimeoutMs);
    // ATI - Product info
    bool GetProductInfo(std::string& info, int timeout_ms = kDefaultTimeoutMs);
    // ATL - Speaker loudness (0-3)
    bool SetSpeakerLoudness(int level, int timeout_ms = kDefaultTimeoutMs);
    // ATM - Speaker mode (0-2)
    bool SetSpeakerMode(int mode, int timeout_ms = kDefaultTimeoutMs);
    // +++ - Switch to command mode
    bool SwitchToCommandMode(int timeout_ms = kDefaultTimeoutMs);
    // ATO - Switch to data mode
    bool SwitchToDataMode(int timeout_ms = kDefaultTimeoutMs);
    // ATQ - Result code presentation mode (0=show, 1=suppress)
    bool SetResultCodePresentation(int mode, int timeout_ms = kDefaultTimeoutMs);
    // ATS0 - Auto answer
    bool SetAutoAnswer(int rings, int timeout_ms = kDefaultTimeoutMs);
    // ATS3 - Command line termination character
    bool SetCommandLineTerminator(int char_code, int timeout_ms = kDefaultTimeoutMs);
    // ATS4 - Response formatting character
    bool SetResponseFormatChar(int char_code, int timeout_ms = kDefaultTimeoutMs);
    // ATS5 - Command line editing character
    bool SetEditingChar(int char_code, int timeout_ms = kDefaultTimeoutMs);
    // ATS6 - Pause before blind dialling (seconds)
    bool SetBlindDialPause(int seconds, int timeout_ms = kDefaultTimeoutMs);
    // ATS7 - Connection completion timeout (seconds)
    bool SetConnectionTimeout(int seconds, int timeout_ms = kDefaultTimeoutMs);
    // ATS8 - Comma dial modifier pause (seconds)
    bool SetCommaDialPause(int seconds, int timeout_ms = kDefaultTimeoutMs);
    // ATS10 - Data carrier lost delay (unit: 0.1s)
    bool SetDataCarrierLostDelay(int tenths, int timeout_ms = kDefaultTimeoutMs);
    // ATV - Response format (0=numeric, 1=verbose)
    bool SetResponseFormat(int mode, int timeout_ms = kDefaultTimeoutMs);
    // ATX - Connect result code format
    bool SetConnectResultFormat(int mode, int timeout_ms = kDefaultTimeoutMs);
    // ATZ - Reset default configuration
    bool ResetDefaultConfiguration(int timeout_ms = kDefaultTimeoutMs);
    // AT&C - DCD function mode
    bool SetDcdMode(int mode, int timeout_ms = kDefaultTimeoutMs);
    // AT&D - DTR function mode
    bool SetDtrMode(int mode, int timeout_ms = kDefaultTimeoutMs);
    // AT&E - CONNECT result code format about speed
    bool SetConnectSpeedFormat(int mode, int timeout_ms = kDefaultTimeoutMs);
    // AT+GCAP - Capabilities
    bool GetCapabilities(std::string& caps, int timeout_ms = kDefaultTimeoutMs);
    // AT+GMI - Manufacturer
    bool GetManufacturer(std::string& info, int timeout_ms = kDefaultTimeoutMs);
    // AT+GMM - Model
    bool GetModel(std::string& info, int timeout_ms = kDefaultTimeoutMs);
    // AT+GMR - Revision
    bool GetRevision(std::string& info, int timeout_ms = kDefaultTimeoutMs);
    // AT+GOI - Global object identification
    bool GetGlobalObjectId(std::string& id, int timeout_ms = kDefaultTimeoutMs);
    // AT+GSN - IMEI
    bool GetIMEI(std::string& imei, int timeout_ms = kDefaultTimeoutMs);
    // AT+ICF - TE-TA control character framing
    bool SetControlCharFraming(int format, int parity, int timeout_ms = kDefaultTimeoutMs);
    // AT+IPR - Fixed baud rate
    bool SetFixedBaudRate(int rate, int timeout_ms = kDefaultTimeoutMs);

   private:
    bool SendAndExpectOk(const char* cmd, std::string& response, int timeout_ms);
};
}  // namespace wrapper