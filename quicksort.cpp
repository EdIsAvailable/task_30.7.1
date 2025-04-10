#include "quicksort.h"

void quicksort(
    std::vector<int>& array, 
    int left, 
    int right, 
    ThreadPool& pool, 
    std::shared_ptr<TaskContext<void>> parent_context, 
    bool make_thread
) {
    // Базовый случай
    if (left >= right) {
        if (parent_context) {
            parent_context->complete_subtask();
        }
        return;
    }
    
    // Выбираем опорный элемент и разделяем массив
    int pivot = array[(left + right) / 2];
    int i = left, j = right;
    
    while (i <= j) {
        while (i <= right && array[i] < pivot) i++;
        while (j >= left && array[j] > pivot) j--;
        if (i <= j) {
            std::swap(array[i], array[j]);
            i++;
            j--;
        }
    }
    
    // Определяем, нужно ли распараллеливать
    // Используем больший порог для параллелизма, чтобы сократить накладные расходы
    bool parallelize = make_thread && (right - left > 100000);
    
    if (parallelize) {
        // Создаем задачу для обработки левой части массива
        auto left_task = [&array, left, j, &pool]() {
            auto task_context = std::make_shared<TaskContext<void>>();
            quicksort(array, left, j, pool, task_context, true);
            task_context->get_future().wait(); // Дожидаемся завершения всех подзадач
        };
        
        // Добавляем задачу в пул
        pool.push_task(left_task);
        
        // Обрабатываем правую часть в текущем потоке
        {
            auto task_context = std::make_shared<TaskContext<void>>();
            quicksort(array, i, right, pool, task_context, true);
            task_context->get_future().wait(); // Дожидаемся завершения всех подзадач
        }
        
        // Уведомляем родителя о завершении
        if (parent_context) {
            parent_context->complete_subtask();
        }
    } else {
        // Последовательная обработка
        
        // Обрабатываем левую часть
        quicksort(array, left, j, pool, nullptr, false);
        
        // Обрабатываем правую часть
        quicksort(array, i, right, pool, nullptr, false);
        
        // Уведомляем родителя о завершении, если он есть
        if (parent_context) {
            parent_context->complete_subtask();
        }
    }
}
