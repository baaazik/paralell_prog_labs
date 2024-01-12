#include <iostream>
#include <iomanip>
#include <mpi.h>
#include <Windows.h>

#define TAG_VALUE       1
#define MAX_NEIGHBOURS  3

// Задание
static const int M = 9;
static const int N = 9;
static const int V = 7;

// Число процессов
static const int K = N + M + (V % 5) * 2;

static int size, rank;

// Создание графа
static void create_graph(MPI_Comm* comm)
{
    const int N_EDGES = (K - 1) * 2;

    int index[K] = {};
    int edges[N_EDGES] = {};
    int count = 0;
    
    for (int i = 0; i < K; i++) {
        if (i % 2 == 0) {
            // Четный ранк
            if (i > 0)
                edges[count++] = i - 2; // ребро i -> i-2

            edges[count++] = i + 1; // ребро i -> i+1

            if (i < K - 2)
                edges[count++] = i + 2; // ребро i -> i+2

            index[i] = count;       // суммарное число ребер
        } else {
            // Нечетный ранк
            edges[count++] = i - 1; // ребро i -> i-1
            index[i] = count;       // суммарное число ребер
        } 
    }

    // Печать индексов и ребер
    if (rank == 0) {
        for (int i = 0; i < K; i++) {
            std::cout << std::setw(2) << index[i] << " ";
        }
        std::cout << std::endl;

        for (int i = 0; i < N_EDGES; i++) {
            std::cout << std::setw(2) << edges[i] << " ";
        }
        std::cout << std::endl;
    }

    MPI_Graph_create(MPI_COMM_WORLD, K, index, edges, false, comm);
}

// Печать соседей каждой веришны
static void print_neighbours(const MPI_Comm &comm)
{
    int num;            // Число соседей
    int neighbours[3];  // Ранги соседей

    std::cout << "Neighbours:" << std::endl;

    for (int i = 0; i < K; i++) {
        // Определяем число соседей и получаем ранги соседей
        MPI_Graph_neighbors_count(comm, i, &num);
        MPI_Graph_neighbors(comm, i, 3, neighbours);

        // Печатаем соседей
        std::cout << "[" << rank << "] (" << std::setw(2) << num << "): ";
        for (int j = 0; j < num; j++) {
            std::cout << std::setw(2) << neighbours[j] << " ";
        }
        std::cout << std::endl;
    }
}

// Отправляет случайное число всем соседям и печатает числа, принятые от соседей
static void send_number_to_neighbours(const MPI_Comm& comm)
{
    int graph_rank;                 // Наш ранг

    int num;                        // Число соседей
    int neighbours[MAX_NEIGHBOURS]; // Ранги соседей

    int value;                      // Наше число
    int values[MAX_NEIGHBOURS];     // Числа, принятые от соседей

    // Получаем наш ранг в коммуникаторе графа
    MPI_Comm_rank(comm, &graph_rank);

    // Определяем число соседей текущего процесса и ранги соседей
    MPI_Graph_neighbors_count(comm, graph_rank, &num);
    MPI_Graph_neighbors(comm, graph_rank, MAX_NEIGHBOURS, neighbours);

    // Генерируем случайное число
    srand(rank);
    value = rand() % 100;

    // Отпралвяем число соседям с помощью блокирующей буферизированной операции
    for (int i = 0; i < num; i++) {
        MPI_Bsend(&value, 1, MPI_INT, neighbours[i], TAG_VALUE, comm);
    }

    // Принимаем значения от соседей с помощью неблокирующих операций
    MPI_Request requests[MAX_NEIGHBOURS];
    MPI_Status  statuses[MAX_NEIGHBOURS];

    for (int i = 0; i < num; i++) {
        MPI_Irecv(&values[i], 1, MPI_INT, neighbours[i],
            TAG_VALUE, comm, &requests[i]);
    }

    // Ожидаем приема значений от соседей
    MPI_Waitall(num, requests, statuses);

    MPI_Barrier(comm);

    Sleep(10 * graph_rank);

    for (int i = 0; i < num; i++) {
        std::cout
            << statuses[i].MPI_SOURCE << " -> "
            << graph_rank << ": " << values[i]
            << std::endl;
    }
}

void run_mpi_graph(int* argc, char*** argv)
{
    MPI_Init(argc, argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Добавляем MPI буфер, чтобы хватило места на Bsend
    size_t buf_size = 1024 * 1024 * 10;
    void* buffer = malloc(buf_size);
    MPI_Buffer_attach(buffer, buf_size);

    if (rank == 0) {
        std::cout << "Required number of processes: " << K << std::endl;
        std::cout << "Number of processes: " << size << std::endl;

        if (size != K) {
            std::cout << "Error" << std::endl;
        }
    }

    if (size != K) {
        return;
    }

    MPI_Comm graph;

    // Создание графа
    create_graph(&graph);

    // Печать соседей
    if (rank == 0) {
        print_neighbours(graph);
    }

    // Отправка значения соседям
    send_number_to_neighbours(graph);

    MPI_Finalize();
}
