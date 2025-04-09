#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <functional>
#include "thread_pool.h"
#include "quicksort.h"

int main() {
    // Количество потоков в пуле
    size_t num_threads = std::thread::hardware_concurrency();
    ThreadPool pool(num_threads);
    std::cout << "Используется " << num_threads << " потоков" << std::endl;
    
    // Размер массива для сортировки
    const size_t size = 10'000'000;
    
    // Создаем и заполняем массив случайными числами
    std::vector<int> array(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000000);
    
    for (size_t i = 0; i < size; ++i) {
        array[i] = dis(gen);
    }
    
    // Засекаем время
    auto start = std::chrono::high_resolution_clock::now();
    
    // Создаем корневой контекст
    auto root_context = std::make_shared<TaskContext<void>>();
    
    // Запускаем сортировку
    quicksort(array, 0, size - 1, pool, root_context);
    
    // Ждем завершения всех задач
    root_context->get_future().wait();
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    
    // Проверка на отсортированность
    bool sorted = true;
    for (size_t i = 1; i < size; ++i) {
        if (array[i] < array[i - 1]) {
            sorted = false;
            break;
        }
    }
    
    std::cout << "Массив " << (sorted ? "отсортирован" : "НЕ отсортирован") << std::endl;
    std::cout << "Время сортировки: " << elapsed.count() << " секунд" << std::endl;
    
    // Теперь сравним с обычной быстрой сортировкой (без распараллеливания)
    // Заново заполняем массив
    for (size_t i = 0; i < size; ++i) {
        array[i] = dis(gen);
    }
    
    start = std::chrono::high_resolution_clock::now();
    
    // Обычная быстрая сортировка без распараллеливания
    std::function<void(std::vector<int>&, int, int)> sequential_quicksort = 
        [&sequential_quicksort](std::vector<int>& arr, int left, int right) {
            if (left >= right) return;
            
            int pivot = arr[(left + right) / 2];
            int i = left, j = right;
            
            while (i <= j) {
                while (arr[i] < pivot) i++;
                while (arr[j] > pivot) j--;
                if (i <= j) {
                    std::swap(arr[i], arr[j]);
                    i++;
                    j--;
                }
            }
            
            sequential_quicksort(arr, left, j);
            sequential_quicksort(arr, i, right);
        };
    
    sequential_quicksort(array, 0, size - 1);
    
    end = std::chrono::high_resolution_clock::now();
    elapsed = end - start;
    
    // Проверка на отсортированность
    sorted = true;
    for (size_t i = 1; i < size; ++i) {
        if (array[i] < array[i - 1]) {
            sorted = false;
            break;
        }
    }
    
    std::cout << "Последовательная сортировка: " << std::endl;
    std::cout << "Массив " << (sorted ? "отсортирован" : "НЕ отсортирован") << std::endl;
    std::cout << "Время сортировки: " << elapsed.count() << " секунд" << std::endl;
    
    return 0;
}