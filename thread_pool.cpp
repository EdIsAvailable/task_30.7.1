#include "thread_pool.h"
#include <algorithm>
#include <random>
#include <chrono>

ThreadPool::ThreadPool(size_t num_threads)
    : stop(false), active_tasks(0), queue_mutexes(num_threads) {
    
    // Создаем очереди задач для каждого потока
    queues.resize(num_threads);
    
    // Запускаем потоки
    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, i, num_threads] {
            std::mt19937 rng(static_cast<unsigned>(std::hash<std::thread::id>()(std::this_thread::get_id())));
            
            while (true) {
                std::function<void()> task;
                std::unique_lock<std::mutex> lock(queue_mutexes[i]);
                
                // Проверка условия завершения
                if (stop && queues[i].empty() && active_tasks == 0) {
                    break;
                }
                
                // Проверяем свою очередь
                if (!queues[i].empty()) {
                    task = std::move(queues[i].front());
                    queues[i].pop();
                    lock.unlock();
                    active_tasks++;
                    task();
                    active_tasks--;
                    continue;
                }
                
                lock.unlock();
                
                // Work stealing: пытаемся украсть задачу из другой очереди
                bool found_task = false;
                std::vector<size_t> indices(num_threads);
                for (size_t j = 0; j < num_threads; ++j) {
                    indices[j] = j;
                }
                std::shuffle(indices.begin(), indices.end(), rng);

                for (size_t idx : indices) {
                    if (idx == i) continue; // Пропускаем свою очередь
                    
                    std::unique_lock<std::mutex> other_lock(queue_mutexes[idx]);
                    if (!queues[idx].empty()) {
                        task = std::move(queues[idx].front());
                        queues[idx].pop();
                        other_lock.unlock();
                        found_task = true;
                        active_tasks++;
                        task();
                        active_tasks--;
                        break;
                    }
                }
                
                // Если задачу не нашли, ждем
                if (!found_task) {
                    std::unique_lock<std::mutex> wait_lock(wait_mutex);
                    condition.wait_for(wait_lock, std::chrono::milliseconds(100), 
                        [this, i] { 
                            return stop || !queues[i].empty() || active_tasks > 0;
                        });
                }
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(wait_mutex);
        stop = true;
    }
    condition.notify_all();
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}