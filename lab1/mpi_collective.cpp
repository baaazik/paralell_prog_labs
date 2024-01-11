#include <algorithm>
#include <iostream>
#include <iomanip>
#include <vector>
#include <mpi.h>
#include <Windows.h>
#include "matmul.h"

#define SIZE 650            // Размер матриц

static double t0;           // Время начала работы программы
static int size, rank;      // Количество процессов и номер процесса

static double total_work_time; // Общее время выполнения полезной работы
double *a, *b, *c;             // Матрицы для умножения

// Какие процессы запускаются на каждой стадии
static const std::vector<std::vector<int>> stages = {
    { 1, 2, 5, 6 },
    { 1, 4, 5 },
    { 2, 3, 7 },
    { 1, 5 }
};

// Проверяет, что текущий процесс должен выполняться на указанной стадии
static bool is_in_stage(const std::vector<int> &stage)
{
    return std::any_of(stage.cbegin(), stage.cend(),
        [](int i) { return i == rank; });
}

// Печать времени начала работы
static void print_start_time(const double* start_buffer,
    const std::vector<int> &stage)
{
    for (auto proc: stage) {
        std::cout
            << "Process " << std::setw(2) << proc
            << " started: " << std::fixed << std::setw(6)
            << std::setprecision(3) << start_buffer[proc]
            << std::endl;
    }
}

// Печать времени окончания работы
static void print_end_time(const double* start_buffer,
    const double* end_buffer,
    const std::vector<int>& stage)
{
    for (auto proc : stage) {
        double duration = end_buffer[proc] - start_buffer[proc];
        std::cout
            << "Process " << std::setw(2) << proc
            << " ended:   " << std::fixed << std::setw(6)
            << std::setprecision(3) << end_buffer[proc]
            << " duration: " << std::fixed << std::setw(6)
            << std::setprecision(3) << duration
            << std::endl;
    }
}

// Работа нулевого процесса
static void do_work_0()
{
    // Выполняем работу - умножаем матрицы
    double t = matmul(a, b, c, SIZE);
    total_work_time += t;
}

// Работа остальных процессов
static void do_work_other()
{
    // Имитация работы - задержка
    Sleep(1000);
}

// Выполнение одной стадии
static void run_stage(const std::vector<int>& procs)
{
    double start_time, end_time;
    double* start_buffer = NULL;
    double* end_buffer = NULL;

    // Выделяем память под буфер для приема времени
    if (rank == 0) {
        start_buffer = new double[size];
        end_buffer = new double[size];
    }

    // Синхронизируем все процессы
    MPI_Barrier(MPI_COMM_WORLD);

    // Процессы отправляют время начала работы нулевому процессу
    start_time = MPI_Wtime() - t0;
    MPI_Gather(&start_time, 1, MPI_DOUBLE,
        start_buffer, 1, MPI_DOUBLE,
        0, MPI_COMM_WORLD);

    // Печать времени начала работы
    if (rank == 0) {
        print_start_time(start_buffer, procs);
    }

    if (rank == 0) {
        // Выполняем рабоут нулевого процесса
        do_work_0();
    } else if (is_in_stage(procs)) {
        // Если ненулнвой процесс должен работать на этой стадии,
        // выполняем работу
        do_work_other();
    }

    // Процессы отправляют время окончания работы нулевому процессу
    end_time = MPI_Wtime() - t0;
    MPI_Gather(&end_time, 1, MPI_DOUBLE,
        end_buffer, 1, MPI_DOUBLE,
        0, MPI_COMM_WORLD);

    // Печать времени начала работы
    if (rank == 0) {
        print_end_time(start_buffer, end_buffer, procs);
        delete[] start_buffer;
        delete[] end_buffer;
    }

}

void run_mpi_collective(int* argc, char*** argv)
{
    MPI_Init(argc, argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        // Инициализация матриц для умножения
        std::cout << "Prepare" << std::endl;
        a = create_matrix(SIZE);
        b = create_matrix(SIZE);
        c = new double[SIZE * SIZE];
        std::cout << "Start" << std::endl;
    }

    t0 = MPI_Wtime();

    // Выполняем все стадии
    for (auto stage : stages) {
        run_stage(stage);
    }

    // Отчёт
    if (rank == 0) {
        double t1 = MPI_Wtime();
        double total_time = t1 - t0;
        std::cout << "Total time: " << std::setprecision(2) << total_time << std::endl;
        std::cout << "Work: " << std::setprecision(2) << (total_work_time / total_time) * 100.0 << "%" << std::endl;
    }

    MPI_Finalize();
}