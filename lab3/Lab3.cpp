#include <iostream>
#include <iomanip>
#include <functional>
#include <omp.h>
#include <stdint.h>
#include "no_vectorizartion.h"

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
    // Выделяем память под массивы
    T* a = new T[size];
    T* b = new T[size];
    T* c = new T[size];

    // Запуск функции и измерение времени работы
    double t0 = omp_get_wtime();
    f(a, b, c, size);
    double duration = omp_get_wtime() - t0;

    // Освобождаем память
    delete[] a;
    delete[] b;
    delete[] c;

    return duration;
}

// Осуществляет измерение времени работы функции для всех типов данных
double run_all_serial(size_t size)
{
    std::cout << "Serial approach\n";
    std::cout << "double: " << std::setprecision(3) << run<double>(size, calc_serial<double>) << "\n";
    std::cout << "float:  " << std::setprecision(3) << run<float>(size, calc_serial<float>) << "\n";
    std::cout << "int64:  " << std::setprecision(3) << run<int64_t>(size, calc_serial<int64_t>) << "\n";
    std::cout << "int32:  " << std::setprecision(3) << run<int32_t>(size, calc_serial<int32_t>) << "\n";
    std::cout << "int16:  " << std::setprecision(3) << run<int16_t>(size, calc_serial<int16_t>) << "\n";
    std::cout << "int8:   " << std::setprecision(3) << run<int8_t>(size, calc_serial<int8_t>) << "\n";
}

int main()
{
    run_all_serial(1024 * 1024 * 100);
}
