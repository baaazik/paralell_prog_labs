#include <iostream>
#include <iomanip>
#include <functional>
#include <omp.h>
#include <stdint.h>
#include "serial.h"
#include "intrinsic.h"

// Тип функции, осуществляющей вычисление по варианту
template<class T>
using F = std::function<void(T*, T*, T*, size_t)>;

// Заполняет массив значениями
template<class T>
void init_array(T* arr, size_t size)
{
    T x0 = rand();
    for (size_t i = 0; i < size; i++) {
        arr[i] = x0 + i % 1000;
    }
}

// Осуществляет измерение времени работы функции
template<class T>
double run(size_t size, F<T> f)
{
    // Выделяем память под массивы с выравниванием, которое требуется для AVX
    T* a = (T*)_aligned_malloc(sizeof(T) * size, 32);
    T* b = (T*)_aligned_malloc(sizeof(T) * size, 32);
    T* c = (T*)_aligned_malloc(sizeof(T) * size, 32);

    // Запуск функции и измерение времени работы
    double t0 = omp_get_wtime();
    f(a, b, c, size);
    double duration = omp_get_wtime() - t0;

    // Освобождаем память
    _aligned_free(a);
    _aligned_free(b);
    _aligned_free(c);

    return duration;
}

// Тестирование последовательной версии
void run_all_serial(size_t size)
{
    std::cout << "Serial approach\n";
    std::cout << "double: " << std::setprecision(3) << run<double>(size, calc_serial<double>) << "\n";
    std::cout << "float:  " << std::setprecision(3) << run<float>(size, calc_serial<float>) << "\n";
    std::cout << "int64:  " << std::setprecision(3) << run<int64_t>(size, calc_serial<int64_t>) << "\n";
    std::cout << "int32:  " << std::setprecision(3) << run<int32_t>(size, calc_serial<int32_t>) << "\n";
    std::cout << "int16:  " << std::setprecision(3) << run<int16_t>(size, calc_serial<int16_t>) << "\n";
    std::cout << "int8:   " << std::setprecision(3) << run<int8_t>(size, calc_serial<int8_t>) << "\n";
    std::cout << std::endl;
}

// Тестирование верссии с интринсиками
void run_all_intrinsic(size_t size)
{
    std::cout << "Intrinsic approach\n";
    std::cout << "double: " << std::setprecision(3) << run<double>(size, calc_intrinsic<double>) << "\n";
    std::cout << "float:  " << std::setprecision(3) << run<float>(size, calc_intrinsic<float>) << "\n";
    std::cout << "int64:  " << std::setprecision(3) << run<int64_t>(size, calc_intrinsic<int64_t>) << "\n";
    std::cout << "int32:  " << std::setprecision(3) << run<int32_t>(size, calc_intrinsic<int32_t>) << "\n";
    std::cout << "int16:  " << std::setprecision(3) << run<int16_t>(size, calc_intrinsic<int16_t>) << "\n";
    std::cout << "int8:   " << std::setprecision(3) << run<int8_t>(size, calc_intrinsic<int8_t>) << "\n";
    std::cout << std::endl;
}


int main()
{
    size_t size = 1024 * 1024 * 100;
    run_all_serial(size);
    run_all_intrinsic(size);
}
