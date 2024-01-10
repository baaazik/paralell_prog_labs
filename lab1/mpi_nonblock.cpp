#include <Windows.h>
#include <mpi.h>
#include <iostream>
#include <iomanip>
#include <functional>
#include <vector>
#include "common.h"
#include "matmul.h"
#include "mpi_block.h"

// Структура с данными, использующимися для приема
struct RecvData {
    int           count;         // Количество приемов
    Notification *notifications; // Принимаемые данные
    MPI_Request  *requests;      // Статусы операции
};

// Размер матрицы
#define SIZE 300

// Время начала работы программы
static double t0;
static int size, rank;


// Блокирующая ртправка сообщения синхронизаици указанному процессу
static void sync_send(int dest)
{
    // std::cout << "[" << rank << "]: send " << dest << std::endl;
    MPI_Send(NULL, 0, MPI_INT, dest, TAG_SYNC, MPI_COMM_WORLD);
    // std::cout << "[" << rank << "]: send " << dest << " ok" << std::endl;
}

// Ожидание сообщения синхронизации от указанного процесса
static void sync_recv(int src)
{
    MPI_Status status;
    // std::cout << "[" << rank << "]: recv " << src << std::endl;
    MPI_Recv(NULL, 0, MPI_INT, src, TAG_SYNC, MPI_COMM_WORLD, &status);
    // std::cout << "[" << rank << "]: recv " << src << " ok" << std::endl;
}

// Блокирующая отправка нулевому процессу уведомления о начале работы
static void notify_start(Notification& n)
{
    MPI_Request request;
    MPI_Status status;

    n.start = MPI_Wtime() - t0;
    n.end = 0;
    n.duration = 0;
    // std::cout << "[" << rank << "]: start" << std::endl;
    MPI_Ibsend(&n, sizeof(Notification), MPI_CHAR, 0, TAG_START, MPI_COMM_WORLD, &request);
    MPI_Wait(&request, &status);
}

// Блокирующая отправка нулевому процессу уведомления об окончании работы
static void notify_end(Notification& n)
{
    MPI_Request request;
    MPI_Status status;

    n.end = MPI_Wtime() - t0;
    n.duration = n.end - n.start;
    // std::cout << "[" << rank << "]: end" << std::endl;
    MPI_Ibsend(&n, sizeof(Notification), MPI_CHAR, 0, TAG_END, MPI_COMM_WORLD, &request);
    MPI_Wait(&request, &status);
}

// Запускает неблокирующий прием уведомлений от процессов
// count - количество операций приема
static RecvData start_notify_receive(int count)
{
    RecvData data;
    data.count = count;
    data.notifications = new Notification[count];
    data.requests = new MPI_Request[count];
    
    for (int i = 0; i < count; i++) {
        // Запускаем асинхронный прием
        MPI_Irecv(&data.notifications[i], sizeof(Notification),
                  MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
                  &data.requests[i]);
    }

    return data;
}

// Печатает сообщение от процесса
static void print_notification(MPI_Status status, Notification n)
{
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

// Проверяет, есть ли сообщения от процессов и печатает их
static void test_notify_receive(const RecvData data)
{
    for (int i = 0; i < data.count; i++) {
        if (data.requests[i] != MPI_REQUEST_NULL) {
            MPI_Status status;
            int flag = 0;

            MPI_Test(&data.requests[i], &flag, &status);
            if (flag) {
                print_notification(status, data.notifications[i]);
            }
        }
    }
}

// Функция имитирует работу процесса - задержка
static void do_work()
{
    Notification n;
    notify_start(n);
    // std::cout << rank << std::endl;
    Sleep(1000);
    notify_end(n);
}

static void task_0()
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

    // Запускаем неблокирующее ожидание сообщений
    int jobs_count = 12;
    int messages_count = jobs_count * 2 + 1;
    RecvData data = start_notify_receive(messages_count);

    // Запускаем неблокирующее ожидание завершения процесса #0
    MPI_Request final_request;
    MPI_Status  final_status;
    int         final_flag;
    MPI_Irecv(NULL, 0, MPI_CHAR, 1, TAG_SYNC, MPI_COMM_WORLD, &final_request);

    // Запуск процессов
    std::cout << "Start" << std::endl;
    sync_send(1);
    sync_send(2);
    sync_send(5);
    sync_send(6);

    
    while(true) {
        // Выполняем работу - умножаем матрицы
        double t = matmul(a, b, c, SIZE);
        // std::cout << "matmul duration " << t << std::endl;
        matmul_count++;
        total_work_time += t;

        // Ожидаем сообщения от процессов 1 - 7
        test_notify_receive(data);
        Sleep(10);

        // Проверяем, что последний процесс завершился и завершаем цикл
        MPI_Test(&final_request, &final_flag, &final_status);
        if (final_flag)
            break;
    }

    std::cout << "End" << std::endl;

    // Отчёт
    double t1 = MPI_Wtime();
    double total_time = t1 - t0;
    std::cout << "Total time: " << std::setprecision(2) << total_time << std::endl;
    std::cout << "Work: " << std::setprecision(2) << (total_work_time / total_time) * 100.0 << "%" << std::endl;
    std::cout << "Matrix multiplication: " << matmul_count << " times" << std::endl;
}

static void dbg_done()
{
    std::cout << "[" << rank << "] done" << std::endl;
}

static void task_1()
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
    // Отправляем аснихронное сообщение процессу 0
    MPI_Request request;
    MPI_Status status;
    MPI_Ibsend(NULL, 0, MPI_CHAR, 0, TAG_SYNC, MPI_COMM_WORLD, &request);
    MPI_Wait(&request, &status);
}

static void task_2()
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

static void task_3()
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

static void task_4()
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

static void task_5()
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

static void task_6()
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

static void task_7()
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
static std::vector<std::function<void()>> tasks = {
    task_0,
    task_1,
    task_2,
    task_3,
    task_4,
    task_5,
    task_6,
    task_7
};

void run_mpi_nonblock(int* argc, char*** argv)
{
    MPI_Init(argc, argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Добавляем MPI буфер, чтобы хватило места на Bsend
    size_t size = 1024 * 1024 * 10;
    void* buffer = malloc(size);
    MPI_Buffer_attach(buffer, size);

    t0 = MPI_Wtime();

    // В зависимости от ранка (номера процесса) запускаем соответствующую задачу
    if (rank < tasks.size()) {
        tasks[rank]();
    }

    MPI_Finalize();
}
