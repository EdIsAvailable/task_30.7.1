#pragma once

#include <atomic>
#include <future>
#include <memory>

// Класс для работы с задачами и их подзадачами
template <typename T>
class TaskContext {
public:
    TaskContext();
    
    // Получить future для ожидания результата
    std::future<T> get_future();
    
    // Увеличить счетчик ожидаемых подзадач
    void add_subtask();
    
    // Уменьшить счетчик подзадач и установить результат, если все подзадачи завершены
    void complete_subtask(T&& result);
    
    // Уменьшить счетчик подзадач без установки результата
    void complete_subtask();
    
private:
    std::shared_ptr<std::promise<T>> promise;
    std::shared_ptr<std::atomic<int>> pending_subtasks;
};

// Специализация для void
template <>
class TaskContext<void> {
public:
    TaskContext();
    
    std::future<void> get_future();
    
    void add_subtask();
    
    void complete_subtask();
    
private:
    std::shared_ptr<std::promise<void>> promise;
    std::shared_ptr<std::atomic<int>> pending_subtasks;
};

// Реализация шаблонного класса TaskContext
template <typename T>
TaskContext<T>::TaskContext() 
    : promise(std::make_shared<std::promise<T>>()), 
      pending_subtasks(std::make_shared<std::atomic<int>>(1)) {}

template <typename T>
std::future<T> TaskContext<T>::get_future() {
    return promise->get_future();
}

template <typename T>
void TaskContext<T>::add_subtask() {
    pending_subtasks->fetch_add(1, std::memory_order_relaxed);
}

template <typename T>
void TaskContext<T>::complete_subtask(T&& result) {
    if (pending_subtasks->fetch_sub(1, std::memory_order_acq_rel) == 1) {
        // Последняя задача, устанавливаем результат
        promise->set_value(std::move(result));
    }
}

template <typename T>
void TaskContext<T>::complete_subtask() {
    if (pending_subtasks->fetch_sub(1, std::memory_order_acq_rel) == 1) {
        // Нет результата для установки, это должна делать родительская задача
    }
}