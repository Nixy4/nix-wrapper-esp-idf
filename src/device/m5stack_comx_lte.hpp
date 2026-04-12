#include "wrapper/uart.hpp"

namespace wrapper
{
    class M5StackComXLTE : public UartDevice
    {
    public:
        M5StackComXLTE(Logger &logger);
        ~M5StackComXLTE();

        bool Init(UartPort &port);
        bool Deinit();
    };
} // namespace wrapper