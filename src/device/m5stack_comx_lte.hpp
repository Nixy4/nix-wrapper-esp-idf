#include "wrapper/uart.hpp"

namespace wrapper
{
    class M5StackComXLTE : public AtDevice
    {
    public:
        M5StackComXLTE(Logger &logger);
        ~M5StackComXLTE();

        bool Init(UartPort &port);
        bool Deinit();

        // Basic control (V.25TER)
        bool Test(int timeout_ms = 3000);
        bool Reset(int timeout_ms = 3000);
        bool SetEchoMode(bool enable, int timeout_ms = 3000);

        // Device information
        bool GetProductInfo(std::string &info, int timeout_ms = 3000);
        bool GetManufacturer(std::string &info, int timeout_ms = 3000);
        bool GetModel(std::string &info, int timeout_ms = 3000);
        bool GetRevision(std::string &info, int timeout_ms = 3000);
        bool GetIMEI(std::string &imei, int timeout_ms = 3000);
        bool GetCapabilities(std::string &caps, int timeout_ms = 3000);

        // Call control
        bool Dial(const std::string &number, int timeout_ms = 10000);
        bool Hangup(int timeout_ms = 3000);
        bool SetAutoAnswer(int rings, int timeout_ms = 3000);

        // Mode switching
        bool SwitchToCommandMode(int timeout_ms = 3000);
        bool SwitchToDataMode(int timeout_ms = 3000);

        // Configuration
        bool SetFixedBaudRate(int rate, int timeout_ms = 3000);

    private:
        bool SendAndExpectOk(const char *cmd, std::string &response, int timeout_ms);
    };
} // namespace wrapper