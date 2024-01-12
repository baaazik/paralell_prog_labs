#include <iostream>
#include <iomanip>
#include <mpi.h>

#pragma pack(1)
struct Shift {
    int source;
    int destination;
};

static const int M = 9;
static const int N = 9;

static int size, rank;

// Печать топологии
static void print_cart(const MPI_Comm& comm)
{
    int size;
    int dims[2];
    int periods[2];
    int processCoord[2];

    // Получаем размер коммутатора и размеры виртуальной топологии
    MPI_Comm_size(comm, &size);
    MPI_Cart_get(comm, 2, dims, periods, processCoord);

    // Проходим по всем координатам и печатаем ранк процесса
    for (int y = 0; y < dims[1]; y++) {
        for (int x = 0; x < dims[0]; x++) {
            // Получаем ранк по координатам
            int rank;
            int coords[] = { x, y };
            MPI_Cart_rank(comm, coords, &rank);
            std::cout << std::setw(2) << rank << " ";
        }
        std::cout << std::endl;
    }
}

// Сдвиг и печать
static void shift_and_print(const MPI_Comm& comm, int dir, int disp)
{
    int indexes[2];
    Shift all_indexes[M * N];

    // Определяем координаты соседей (источника сдвига, получателя свдига)
    MPI_Cart_shift(comm, dir, disp, &indexes[0], &indexes[1]);

    // Отправляем все координаты нулевому процессу, чтобы он их напечатал
    MPI_Gather(indexes, 2, MPI_INT, all_indexes, 2, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::cout << "Shift, dir=" << dir << ", disp=" << disp << std::endl;
        // Проходим по всем координатам и печатаем ранк процесса
        for (int y = 0; y < N; y++) {
            for (int x = 0; x < M; x++) {
                // Получаем ранк по координатам
                int rank;
                int coords[] = { x, y };
                MPI_Cart_rank(comm, coords, &rank);
                std::cout
                    << std::setw(2) << all_indexes[rank].source << " "
                    << std::setw(2) << rank << " "
                    << std::setw(2) << all_indexes[rank].destination << " | ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
}

// Создание апериодической топологии
static void task_1()
{
    MPI_Comm comm;

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        std::cout << "Create aperiodic topology" << std::endl;
    }

    // Создание топологии
    int dims[] = { N, M };
    int periods[] = { false, false };
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, false, &comm);

    // Печать топологии
    if (rank == 0) {
        std::cout << "Topology:" << std::endl;
        print_cart(comm);
        std::cout << std::endl;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // Сдвиг вправо на 1
    shift_and_print(comm, 0, 1);

    // Сдвиг вправо на 2
    shift_and_print(comm, 0, 2);

    // Сдвиг вниз на 1
    shift_and_print(comm, 1, 1);

    // Сдвиг вниз на 2
    shift_and_print(comm, 1, 2);
}

// Создание периодической топологии
static void task_2()
{
    MPI_Comm comm;
    int ret;

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        std::cout << "Create periodic topology" << std::endl;
    }

    // Создание топологии
    int dims[] = { N, M };
    int periods[] = { true, true };
    ret = MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, false, &comm);

    // Печать топологии
    if (rank == 0) {
        std::cout << "Topology:" << std::endl;
        print_cart(comm);
        std::cout << std::endl;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // Сдвиг вправо на 1
    shift_and_print(comm, 0, 1);

    // Сдвиг вправо на 2
    shift_and_print(comm, 0, 2);

    // Сдвиг вниз на 1
    shift_and_print(comm, 1, 1);

    // Сдвиг вниз на 2
    shift_and_print(comm, 1, 2);
}

// Разщепление топологии на строки
static void task_3()
{
    MPI_Comm comm, line_comm;
    int line_size, line_rank;
    int value;

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        std::cout << "Separate topology" << std::endl;
    }

    // Создание топологии
    int dims[] = { N, M };
    int periods[] = { false, false };
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, false, &comm);

    // Разделяем топологию на одномерные
    int remainDims[] = { 1, 0 };
    MPI_Cart_sub(comm, remainDims, &line_comm);
    MPI_Comm_size(line_comm, &line_size);
    MPI_Comm_rank(line_comm, &line_rank);

    // Из главного процесса каждой линии перешлем число в остальные процессы этой линии
    if (line_rank == 0) {
        // В качестве числа возьмём глобальный ранк главного процесса линии
        value = rank;
    }

    // Разсылаем всем в линии
    MPI_Bcast(&value, 1, MPI_INT, 0, line_comm);

    // Печать
    std::cout
        << "rank: " << std::setw(2) << rank
        << ", line_rank: " << std::setw(2) << line_rank
        << ", value: " << value << std::endl;
}

void run_mpi_cartesian(int* argc, char*** argv)
{
    MPI_Init(argc, argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        std::cout << "Number of processes: " << size << std::endl;
    }

    task_1();
    task_2();
    task_3();

    MPI_Finalize();
}
