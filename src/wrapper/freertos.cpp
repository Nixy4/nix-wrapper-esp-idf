#include "freertos.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace wrapper
{

void Delay(uint32_t milliseconds) { vTaskDelay(pdMS_TO_TICKS(milliseconds)); }

void DelayTicks(uint32_t ticks) { vTaskDelay(ticks); }

void DelayUntil(uint32_t* previous_wake_time, uint32_t time_increment_ms)
{
    vTaskDelayUntil(previous_wake_time, pdMS_TO_TICKS(time_increment_ms));
}

void DelayUntilTicks(uint32_t* previous_wake_time, uint32_t time_increment_ticks)
{
    vTaskDelayUntil(previous_wake_time, time_increment_ticks);
}

// Task Implementation
Task::Task(const std::string& name, std::function<void(void*)> function, void* arg,
           uint32_t stack_depth, UBaseType_t priority)
    : name_(name),
      function_(function),
      arg_(arg),
      stack_depth_(stack_depth),
      priority_(priority),
      handle_(nullptr)
{
}

Task::~Task() {}

bool Task::Create()
{
    BaseType_t result = xTaskCreate(
        [](void* pvParameters)
        {
            Task* task = static_cast<Task*>(pvParameters);
            task->function_(task->arg_);
            vTaskDelete(nullptr);
        },
        name_.c_str(), stack_depth_, this, priority_, &handle_);
    return result == pdPASS;
}

void Task::Delete()
{
    if (handle_ != nullptr)
    {
        vTaskDelete(handle_);
        handle_ = nullptr;
    }
}

void Task::Suspend()
{
    if (handle_ != nullptr)
    {
        vTaskSuspend(handle_);
    }
}

void Task::Resume()
{
    if (handle_ != nullptr)
    {
        vTaskResume(handle_);
    }
}

void Task::SetPriority(UBaseType_t new_priority)
{
    if (handle_ != nullptr)
    {
        vTaskPrioritySet(handle_, new_priority);
        priority_ = new_priority;
    }
}

std::string Task::GetName() const { return name_; }

uint32_t Task::GetStackDepth() const { return stack_depth_; }

UBaseType_t Task::GetPriority() const { return priority_; }

TaskHandle_t Task::GetHandle() const { return handle_; }

// Semaphore Implementation
Semaphore::Semaphore() : handle_(nullptr) {}

Semaphore::~Semaphore()
{
    if (handle_ != nullptr)
    {
        vSemaphoreDelete(handle_);
        handle_ = nullptr;
    }
}

bool Semaphore::Take(TickType_t wait_ticks)
{
    if (handle_ == nullptr)
        return false;
    return xSemaphoreTake(handle_, wait_ticks) == pdTRUE;
}

bool Semaphore::Give()
{
    if (handle_ == nullptr)
        return false;
    return xSemaphoreGive(handle_) == pdTRUE;
}

bool Semaphore::TakeFromISR(BaseType_t* pxHigherPriorityTaskWoken)
{
    if (handle_ == nullptr)
        return false;
    return xSemaphoreTakeFromISR(handle_, pxHigherPriorityTaskWoken) == pdTRUE;
}

bool Semaphore::GiveFromISR(BaseType_t* pxHigherPriorityTaskWoken)
{
    if (handle_ == nullptr)
        return false;
    return xSemaphoreGiveFromISR(handle_, pxHigherPriorityTaskWoken) == pdTRUE;
}

// BinarySemaphore Implementation
BinarySemaphore::BinarySemaphore() { handle_ = xSemaphoreCreateBinary(); }

// CountingSemaphore Implementation
CountingSemaphore::CountingSemaphore(UBaseType_t max_count, UBaseType_t initial_count)
{
    handle_ = xSemaphoreCreateCounting(max_count, initial_count);
}

// Mutex Implementation
Mutex::Mutex() { handle_ = xSemaphoreCreateMutex(); }

// RecursiveMutex Implementation
RecursiveMutex::RecursiveMutex() { handle_ = xSemaphoreCreateRecursiveMutex(); }

bool RecursiveMutex::Take(TickType_t wait_ticks)
{
    if (handle_ == nullptr)
        return false;
    return xSemaphoreTakeRecursive(handle_, wait_ticks) == pdTRUE;
}

bool RecursiveMutex::Give()
{
    if (handle_ == nullptr)
        return false;
    return xSemaphoreGiveRecursive(handle_) == pdTRUE;
}

// EventGroup Implementation
EventGroup::EventGroup() { handle_ = xEventGroupCreate(); }

EventGroup::~EventGroup()
{
    if (handle_ != nullptr)
    {
        vEventGroupDelete(handle_);
        handle_ = nullptr;
    }
}

EventBits_t EventGroup::WaitBits(const EventBits_t bits_to_wait_for, const bool clear_on_exit,
                                 const bool wait_for_all_bits, TickType_t wait_ticks)
{
    if (handle_ == nullptr)
        return 0;
    return xEventGroupWaitBits(handle_, bits_to_wait_for, clear_on_exit, wait_for_all_bits,
                               wait_ticks);
}

EventBits_t EventGroup::SetBits(const EventBits_t bits_to_set)
{
    if (handle_ == nullptr)
        return 0;
    return xEventGroupSetBits(handle_, bits_to_set);
}

EventBits_t EventGroup::SetBitsFromISR(const EventBits_t bits_to_set,
                                       BaseType_t* pxHigherPriorityTaskWoken)
{
    if (handle_ == nullptr)
        return 0;
    return xEventGroupSetBitsFromISR(handle_, bits_to_set, pxHigherPriorityTaskWoken);
}

EventBits_t EventGroup::ClearBits(const EventBits_t bits_to_clear)
{
    if (handle_ == nullptr)
        return 0;
    return xEventGroupClearBits(handle_, bits_to_clear);
}

EventBits_t EventGroup::ClearBitsFromISR(const EventBits_t bits_to_clear)
{
    if (handle_ == nullptr)
        return 0;
    return xEventGroupClearBitsFromISR(handle_, bits_to_clear);
}

EventBits_t EventGroup::GetBits() const
{
    if (handle_ == nullptr)
        return 0;
    return xEventGroupGetBits(handle_);
}

EventBits_t EventGroup::GetBitsFromISR() const
{
    if (handle_ == nullptr)
        return 0;
    return xEventGroupGetBitsFromISR(handle_);
}

EventBits_t EventGroup::Sync(const EventBits_t bits_to_set, const EventBits_t bits_to_wait_for,
                             TickType_t wait_ticks)
{
    if (handle_ == nullptr)
        return 0;
    return xEventGroupSync(handle_, bits_to_set, bits_to_wait_for, wait_ticks);
}

EventGroupHandle_t EventGroup::GetHandle() const { return handle_; }

bool EventGroup::IsValid() const { return handle_ != nullptr; }

SemaphoreHandle_t Semaphore::GetHandle() const { return handle_; }

bool Semaphore::IsValid() const { return handle_ != nullptr; }

}  // namespace wrapper
