#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <memory>
#include "task_context.h"

// Пул потоков с поддержкой work stealing и вложенных задач
class ThreadPool {
public:
    ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
    ~ThreadPool();

    // Метод для добавления задачи в пул
    template<typename F, typename... Args>
    auto push_task(F&& f, Args&&... args) {
        using return_type = std::invoke_result_t<F, Args...>;
        
        auto task_context = std::make_shared<TaskContext<return_type>>();
        auto future = task_context->get_future();
        
        size_t idx = worker_index++;
        idx %= queues.size();
        
        auto task_function = [task_context, f = std::forward<F>(f), args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
            if constexpr (std::is_void_v<return_type>) {
                std::apply(f, args);
                task_context->complete_subtask();
            } else {
                auto result = std::apply(f, args);
                task_context->complete_subtask(std::move(result));
            }
        };
        
        {
            std::lock_guard<std::mutex> lock(queue_mutexes[idx]);
            queues[idx].emplace(std::move(task_function));
        }
        
        condition.notify_one();
        return future;
    }
    
    // Метод для получения текущего контекста задачи
    template<typename T>
    std::shared_ptr<TaskContext<T>> create_task_context() {
        return std::make_shared<TaskContext<T>>();
    }

private:
    std::vector<std::thread> threads;
    std::vector<std::queue<std::function<void()>>> queues;
    std::vector<std::mutex> queue_mutexes;
    std::mutex wait_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
    std::atomic<size_t> worker_index{0};
    std::atomic<int> active_tasks;
};