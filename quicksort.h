#pragma once

#include <vector>
#include <memory>
#include "thread_pool.h"

// Быстрая сортировка с использованием пула потоков
void quicksort(
    std::vector<int>& array, 
    int left, 
    int right, 
    ThreadPool& pool, 
    std::shared_ptr<TaskContext<void>> parent_context = nullptr, 
    bool make_thread = true
);