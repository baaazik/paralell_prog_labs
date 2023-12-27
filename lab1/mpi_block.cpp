#include <Windows.h>
#include <mpi.h>
#include <iostream>
#include <iomanip>
#include <functional>
#include <vector>
#include "common.h"
#include "matmul.h"
#include "mpi_block.h"

// Размер матрицы
#define SIZE 300

// Время начала работы программы
double t0;
int size, rank;


// Блокирующая ртправка сообщения синхронизаици указанному процессу
void sync_send(int dest)
{
    // std::cout << "[" << rank << "]: send " << dest << std::endl;
    MPI_Send(NULL, 0, MPI_INT, dest, TAG_SYNC, MPI_COMM_WORLD);
    // std::cout << "[" << rank << "]: send " << dest << " ok" << std::endl;
}

// Ожидание сообщения синхронизации от указанного процесса
void sync_recv(int src)
{
    MPI_Status status;
    // std::cout << "[" << rank << "]: recv " << src << std::endl;
    MPI_Recv(NULL, 0, MPI_INT, src, TAG_SYNC, MPI_COMM_WORLD, &status);
    // std::cout << "[" << rank << "]: recv " << src << " ok" << std::endl;
}

// Блокирующая отправка нулевому процессу уведомления о начале работы
void notify_start(Notification& n)
{
    //Notification n{ start - t0, 0, 0 };
    n.start = MPI_Wtime() - t0;
    n.end = 0;
    n.duration = 0;
    // std::cout << "[" << rank << "]: start" << std::endl;
    MPI_Send(&n, sizeof(n), MPI_CHAR, 0, TAG_START, MPI_COMM_WORLD);
}

// Блокирующая отправка нулевому процессу уведомления об окончании работы
void notify_end(Notification& n)
{
    n.end = MPI_Wtime() - t0;
    n.duration = n.end - n.start;
    // std::cout << "[" << rank << "]: end" << std::endl;
    MPI_Send(&n, sizeof(n), MPI_CHAR, 0, TAG_END, MPI_COMM_WORLD);
}

// Блокирующий прием уведомлений от процессов
void notify_receive()
{
    MPI_Status   status;
    Notification n;

    MPI_Recv(&n, sizeof(n), MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    if (status.MPI_TAG == TAG_START) {
        // Печать сообщения о начале работы
        std::cout
            << "Process " << std::setw(2) << status.MPI_SOURCE
            << " started: " << std::fixed << std::setw(6)
            << std::setprecision(3) << n.start
            << std::endl;
    }
    else if (status.MPI_TAG == TAG_END) {
        // Печать сообщения об окончании работы
        std::cout
            << "Process " << std::setw(2) << status.MPI_SOURCE
            << " ended:   " << std::fixed << std::setw(6)
            << std::setprecision(3) << n.end
            << " duration: " << std::fixed << std::setw(6)
            << std::setprecision(3) << n.duration
            << std::endl;
    }
}

// Функция имитирует работу процесса - задержка
void do_work()
{
    Notification n;
    notify_start(n);
    // std::cout << rank << std::endl;
    Sleep(1000);
    notify_end(n);
}

void task_0()
{
    std::cout << "Work with blocking operations" << std::endl;

    // Подготовка матриц
    std::cout << "Prepare" << std::endl;
    double* a = create_matrix(SIZE);
    double* b = create_matrix(SIZE);
    double* c = new double[SIZE * SIZE];

    int matmul_count = 0;       // количество умножений матриц
    double total_work_time = 0; // общее время выполнения работы
    double t0 = MPI_Wtime();

    // Запуск процессов
    std::cout << "Start" << std::endl;
    sync_send(1);
    sync_send(2);
    sync_send(5);
    sync_send(6);

    int jobs_count = 12;
    int messages_count = jobs_count * 2;
    for (int i = 0; i < messages_count + 1; i++) {
        // Выполняем работу - умножаем матрицы
        double t = matmul(a, b, c, SIZE);
        // std::cout << "matmul duration " << t << std::endl;
        matmul_count++;
        total_work_time += t;

        // Ожидаем сообщения от процессов 1 - 7
        notify_receive();
    }

    // Ожидание последнего процесса
    sync_recv(1);
    std::cout << "End" << std::endl;

    // Отчёт
    double t1 = MPI_Wtime();
    double total_time = t1 - t0;
    std::cout << "Total time: " << std::setprecision(2) << total_time << std::endl;
    std::cout << "Work: " << std::setprecision(2) << (total_work_time / total_time) * 100.0 << "%" << std::endl;
    std::cout << "Matrix multiplication: " << matmul_count << " times" << std::endl;
}

void dbg_done()
{
    std::cout << "[" << rank << "] done" << std::endl;
}

void task_1()
{
    // Точка синхронизации 1
    // 0 -> (1, 2, 5, 6)
    sync_recv(0);
    do_work();

    // Точка синхронизации 2
    // (1, 5) -> (1, 4, 5)
    sync_send(5);
    sync_recv(5);
    // send self
    sync_send(4);
    do_work();

    // Точка синхронизации 4
    // (1, 2, 4) -> 1
    sync_recv(2);
    sync_recv(4);
    // send self
    do_work();

    // Точка синхронизации 5
    sync_send(0);
}

void task_2()
{
    // Точка синхронизации 1
    // 0 -> (1, 2, 5, 6)
    sync_recv(0);
    do_work();

    // Точка синхронизации 3
    // (2, 5, 6) -> (2, 3, 7)
    sync_recv(5);
    sync_recv(6);
    // send self
    sync_send(3);
    sync_send(7);
    do_work();

    // Точка синхронизации 4
    // (1, 2, 4) -> 1
    sync_send(1);
}

void task_3()
{
    // Точка синхронизации 3
    // (2, 5, 6) -> (2, 3, 7)
    sync_recv(2);
    sync_recv(5);
    sync_recv(6);
    do_work();

    // Точка синхронизации 5
    sync_send(0);
}

void task_4()
{
    // Точка синхронизации 2
    // (1, 5) -> (1, 4, 5)
    sync_recv(1);
    sync_recv(5);
    do_work();

    // Точка синхронизации 4
    // (1, 2, 4) -> 1
    sync_send(1);
}

void task_5()
{
    // Точка синхронизаци 1
    // 0 -> (1, 2, 5, 6)
    sync_recv(0);
    do_work();

    // Точка синхронизации 2
    // (1, 5) -> (1, 4, 5)
    sync_recv(1);
    sync_send(1);
    sync_send(4);
    do_work();

    // Точка синхронизации 3
    // (2, 5, 6) -> (2, 3, 7)
    sync_send(2);
    sync_send(3);
    sync_send(7);

    // Точка синхронизации 6
    // 7 -> 5
    sync_recv(7);
    do_work();

    // Точка синхронизации 5
    sync_send(0);
}

void task_6()
{
    // Точка синхронизации 1
    // 0 -> (1, 2, 5, 6)
    sync_recv(0);
    do_work();

    // Точка синхронизации 3
    // (2, 5, 6) -> (2, 3, 7)
    sync_send(2);
    sync_send(3);
    sync_send(7);
}

void task_7()
{
    // Точка синхронизации 3
    // (2, 5, 6) -> (2, 3, 7)
    sync_recv(2);
    sync_recv(5);
    sync_recv(6);
    do_work();

    // Точка синхронизации 6
    // 7 -> 5
    sync_send(5);
}

// Список задач
std::vector<std::function<void()>> tasks = {
    task_0,
    task_1,
    task_2,
    task_3,
    task_4,
    task_5,
    task_6,
    task_7
};

void run_mpi_block(int* argc, char*** argv)
{
    MPI_Init(argc, argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    t0 = MPI_Wtime();

    // В зависимости от ранка (номера процесса) запускаем соответствующую задачу
    if (rank < tasks.size()) {
        tasks[rank]();
    }

    MPI_Finalize();
}