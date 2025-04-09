#include "quicksort.h"

void quicksort(
    std::vector<int>& array, 
    int left, 
    int right, 
    ThreadPool& pool, 
    std::shared_ptr<TaskContext<void>> parent_context, 
    bool make_thread
) {
    if (left >= right) {
        if (parent_context) {
            parent_context->complete_subtask();
        }
        return;
    }
    
    // Опорный элемент
    int pivot = array[(left + right) / 2];
    
    // Разделение массива
    int i = left, j = right;
    while (i <= j) {
        while (array[i] < pivot)
            i++;
        while (array[j] > pivot)
            j--;
        if (i <= j) {
            std::swap(array[i], array[j]);
            i++;
            j--;
        }
    }
    
    int left_bound = j;
    int right_bound = i;
    
    // Определяем, нужно ли распараллеливать
    bool parallelize = make_thread && (right_bound - left > 100000);
    
    if (parallelize) {
        // Создаем контекст для подзадачи, если родительский контекст существует
        if (parent_context) {
            parent_context->add_subtask();
        }
        
        // Создаем новый контекст для левой части
        auto left_context = std::make_shared<TaskContext<void>>();
        
        // Отправляем задачу сортировки левой части в пул
        auto task = [&array, left, left_bound, &pool, left_context]() {
            quicksort(array, left, left_bound, pool, left_context, true);
        };
        
        pool.push_task(task);
        
        // Сортируем правую часть в текущем потоке
        quicksort(array, right_bound, right, pool, parent_context, true);
        
        // Ждем завершения левой части
        left_context->get_future().wait();
    } else {
        // Синхронная сортировка обеих частей
        quicksort(array, left, left_bound, pool, parent_context, false);
        quicksort(array, right_bound, right, pool, parent_context, false);
    }
    
    if (parent_context && !parallelize) {
        parent_context->complete_subtask();
    }
}