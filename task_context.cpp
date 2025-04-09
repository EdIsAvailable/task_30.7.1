#include "task_context.h"

// Реализация специализации TaskContext<void>
TaskContext<void>::TaskContext() 
    : promise(std::make_shared<std::promise<void>>()), 
      pending_subtasks(std::make_shared<std::atomic<int>>(1)) {}

std::future<void> TaskContext<void>::get_future() {
    return promise->get_future();
}

void TaskContext<void>::add_subtask() {
    pending_subtasks->fetch_add(1, std::memory_order_relaxed);
}

void TaskContext<void>::complete_subtask() {
    if (pending_subtasks->fetch_sub(1, std::memory_order_acq_rel) == 1) {
        promise->set_value();
    }
}