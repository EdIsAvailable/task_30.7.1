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
/*

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
    */