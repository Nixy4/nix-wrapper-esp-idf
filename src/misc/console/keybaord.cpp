#include "misc/console/keyboard.hpp"

#include <cstdio>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

namespace wrapper
{

namespace
{
constexpr const char* kCommandName = "kbdmon";
constexpr const char* kCommandHelp = "Monitor keyboard input; press Ctrl+C or ESC to stop";

}  // namespace

ConsoleCommandKeyboardMonitor& ConsoleCommandKeyboardMonitor::GetInstance()
{
    static ConsoleCommandKeyboardMonitor instance;
    return instance;
}

ConsoleCommandKeyboardMonitor::ConsoleCommandKeyboardMonitor()
: ConsoleCommand(kCommandName, kCommandHelp, nullptr, &ConsoleCommandKeyboardMonitor::CommandEntry)
{
    (void)EnsureQueue();
}

ConsoleCommandKeyboardMonitor::~ConsoleCommandKeyboardMonitor()
{
    if (key_queue_ != nullptr)
    {
        vQueueDelete(key_queue_);
        key_queue_ = nullptr;
    }
}

int ConsoleCommandKeyboardMonitor::CommandEntry(int argc, char** argv)
{
    return ConsoleCommandKeyboardMonitor::GetInstance().Run(argc, argv);
}

int ConsoleCommandKeyboardMonitor::Run(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    if (!EnsureQueue())
    {
        std::printf("kbdmon queue init failed\n");
        return ESP_FAIL;
    }

    if (monitoring_.exchange(true))
    {
        std::printf("kbdmon already running\n");
        return ESP_FAIL;
    }

    std::printf("kbdmon started: press keys to enqueue. Ctrl+C or ESC to stop.\n");
    std::printf("note: while running, shell command line is paused.\n");

    while (monitoring_.load())
    {
        const int ch = std::getchar();
        if (ch == EOF)
        {
            continue;
        }

        const char key = static_cast<char>(ch & 0xFF);
        if (key == 0x03 || key == 0x1B)
        {
            monitoring_.store(false);
            break;
        }

        (void)PushKey(key);
    }

    monitoring_.store(false);
    std::printf("kbdmon stopped\n");
    return ESP_OK;
}

bool ConsoleCommandKeyboardMonitor::EnsureQueue()
{
    if (key_queue_ != nullptr)
    {
        return true;
    }
    key_queue_ = xQueueCreate(static_cast<UBaseType_t>(kQueueDepth), sizeof(char));
    return key_queue_ != nullptr;
}

bool ConsoleCommandKeyboardMonitor::PushKey(char key)
{
    if (!EnsureQueue())
    {
        return false;
    }

    if (xQueueSend(key_queue_, &key, 0) == pdTRUE)
    {
        return true;
    }

    char dropped = 0;
    (void)xQueueReceive(key_queue_, &dropped, 0);
    return xQueueSend(key_queue_, &key, 0) == pdTRUE;
}

char ConsoleCommandKeyboardMonitor::GetKeyFromQueue()
{
    if (!EnsureQueue())
    {
        return '\0';
    }

    char key = '\0';
    if (xQueueReceive(key_queue_, &key, 0) == pdTRUE)
    {
        return key;
    }
    return '\0';
}

}  // namespace wrapper