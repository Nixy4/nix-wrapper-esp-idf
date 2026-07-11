#pragma once

#include <atomic>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "local/console.hpp"

namespace wrapper
{

class ConsoleCommandKeyboardMonitor : public ConsoleCommand
{
   private:
    ConsoleCommandKeyboardMonitor(const ConsoleCommandKeyboardMonitor&) = delete;
    ConsoleCommandKeyboardMonitor& operator=(const ConsoleCommandKeyboardMonitor&) = delete;
    ConsoleCommandKeyboardMonitor(ConsoleCommandKeyboardMonitor&&) = delete;
    ConsoleCommandKeyboardMonitor& operator=(ConsoleCommandKeyboardMonitor&&) = delete;

    ConsoleCommandKeyboardMonitor();
    ~ConsoleCommandKeyboardMonitor();

    static int CommandEntry(int argc, char** argv);
    int Run(int argc, char** argv);

    bool EnsureQueue();
    bool PushKey(char key);

    static constexpr size_t kQueueDepth = 128;

    QueueHandle_t key_queue_ = nullptr;
    std::atomic<bool> monitoring_{false};

   public:
    static ConsoleCommandKeyboardMonitor& GetInstance();

    char GetKeyFromQueue();
};

}  // namespace wrapper
