п»ї#include <Windows.h>
#include <mpi.h>
#include <iostream>
#include <iomanip>
#include <functional>
#include <vector>
#include "matmul.h"

/* РџСЂРѕС†РµСЃСЃ 0 РІС‹РїРѕР»РЅСЏРµС‚ РЅРµРєСѓСЋ СЂР°Р±РѕС‚Сѓ.
 * РџСЂРѕС†РµСЃСЃС‹ 1-N РІС‹РїРѕР»РЅСЏСЋС‚СЃСЏ РІ РїРѕСЂСЏРґРєРµ, РѕРїСЂРµРґРµР»РµРЅРЅС‹Рј РіСЂР°С„РѕРј РІ РёРЅРґРёРІРёРґСѓР°Р»СЊРЅРѕРј
 * РІР°СЂРёР°РЅС‚Рµ.  */

#define SIZE 100

#define TAG_SYNC  1
#define TAG_START 2
#define TAG_END   3

 // РЎС‚СЂСѓРєС‚СѓСЂР°, РѕС‚РїСЂР°РІР»СЏРµРјР°СЏ i-Рј РїСЂРѕС†РµСЃСЃРѕРј 0-РјСѓ РїСЂРѕС†РµСЃСЃСѓ РґР»СЏ РѕРїРѕРІРµС‰РЅРёСЏ РѕР±
 // РѕРєРѕРЅС‡Р°РЅРёРё СЂР°Р±РѕС‚С‹
struct Notification {
    double start;
    double end;
    double duration;
};

// Р’СЂРµРјСЏ РЅР°С‡Р°Р»Р° СЂР°Р±РѕС‚С‹ РїСЂРѕРіСЂР°РјРјС‹
double t0;
int size, rank;


// Р‘Р»РѕРєРёСЂСѓСЋС‰Р°СЏ СЂС‚РїСЂР°РІРєР° СЃРѕРѕР±С‰РµРЅРёСЏ СЃРёРЅС…СЂРѕРЅРёР·Р°РёС†Рё СѓРєР°Р·Р°РЅРЅРѕРјСѓ РїСЂРѕС†РµСЃСЃСѓ
void sync_send(int dest)
{
    // std::cout << "[" << rank << "]: send " << dest << std::endl;
    MPI_Send(NULL, 0, MPI_INT, dest, TAG_SYNC, MPI_COMM_WORLD);
    // std::cout << "[" << rank << "]: send " << dest << " ok" << std::endl;
}

// РћР¶РёРґР°РЅРёРµ СЃРѕРѕР±С‰РµРЅРёСЏ СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё РѕС‚ СѓРєР°Р·Р°РЅРЅРѕРіРѕ РїСЂРѕС†РµСЃСЃР°
void sync_recv(int src)
{
    MPI_Status status;
    // std::cout << "[" << rank << "]: recv " << src << std::endl;
    MPI_Recv(NULL, 0, MPI_INT, src, TAG_SYNC, MPI_COMM_WORLD, &status);
    // std::cout << "[" << rank << "]: recv " << src << " ok" << std::endl;
}

// Р‘Р»РѕРєРёСЂСѓСЋС‰Р°СЏ РѕС‚РїСЂР°РІРєР° РЅСѓР»РµРІРѕРјСѓ РїСЂРѕС†РµСЃСЃСѓ СѓРІРµРґРѕРјР»РµРЅРёСЏ Рѕ РЅР°С‡Р°Р»Рµ СЂР°Р±РѕС‚С‹
void notify_start(Notification& n)
{
    //Notification n{ start - t0, 0, 0 };
    n.start = MPI_Wtime() - t0;
    n.end = 0;
    n.duration = 0;
    // std::cout << "[" << rank << "]: start" << std::endl;
    MPI_Send(&n, sizeof(n), MPI_CHAR, 0, TAG_START, MPI_COMM_WORLD);
}

// Р‘Р»РѕРєРёСЂСѓСЋС‰Р°СЏ РѕС‚РїСЂР°РІРєР° РЅСѓР»РµРІРѕРјСѓ РїСЂРѕС†РµСЃСЃСѓ СѓРІРµРґРѕРјР»РµРЅРёСЏ РѕР± РѕРєРѕРЅС‡Р°РЅРёРё СЂР°Р±РѕС‚С‹
void notify_end(Notification& n)
{
    n.end = MPI_Wtime() - t0;
    n.duration = n.end - n.start;
    // std::cout << "[" << rank << "]: end" << std::endl;
    MPI_Send(&n, sizeof(n), MPI_CHAR, 0, TAG_END, MPI_COMM_WORLD);
}

// Р‘Р»РѕРєРёСЂСѓСЋС‰РёР№ РїСЂРёРµРј СѓРІРµРґРѕРјР»РµРЅРёР№ РѕС‚ РїСЂРѕС†РµСЃСЃРѕРІ
void notify_receive()
{
    MPI_Status   status;
    Notification n;

    MPI_Recv(&n, sizeof(n), MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    if (status.MPI_TAG == TAG_START) {
        // РџРµС‡Р°С‚СЊ СЃРѕРѕР±С‰РµРЅРёСЏ Рѕ РЅР°С‡Р°Р»Рµ СЂР°Р±РѕС‚С‹
        std::cout
            << "Process " << std::setw(2) << status.MPI_SOURCE
            << " started: " << std::fixed << std::setw(6)
                << std::setprecision(3) << n.start
            << std::endl;
    } else if (status.MPI_TAG == TAG_END) {
        // РџРµС‡Р°С‚СЊ СЃРѕРѕР±С‰РµРЅРёСЏ РѕР± РѕРєРѕРЅС‡Р°РЅРёРё СЂР°Р±РѕС‚С‹
        std::cout
            << "Process " << std::setw(2) << status.MPI_SOURCE
            << " ended:   " << std::fixed << std::setw(6)
                << std::setprecision(3) << n.end
            << " duration: " << std::fixed <<  std::setw(6)
                << std::setprecision(3) << n.duration
            << std::endl;
    }
}

// Р¤СѓРЅРєС†РёСЏ РёРјРёС‚РёСЂСѓРµС‚ СЂР°Р±РѕС‚Сѓ РїСЂРѕС†РµСЃСЃР° - Р·Р°РґРµСЂР¶РєР°
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
    // РџРѕРґРіРѕС‚РѕРІРєР° РјР°С‚СЂРёС†
    std::cout << "Prepare" << std::endl;
    double *a = create_matrix(SIZE);
    double *b = create_matrix(SIZE);
    double* c = new double[SIZE * SIZE];

    int matmul_count = 0;       // РєРѕР»РёС‡РµСЃС‚РІРѕ СѓРјРЅРѕР¶РµРЅРёР№ РјР°С‚СЂРёС†
    double total_work_time = 0; // РѕР±С‰РµРµ РІСЂРµРјСЏ РІС‹РїРѕР»РЅРµРЅРёСЏ СЂР°Р±РѕС‚С‹
    double t0 = MPI_Wtime();

    // Р—Р°РїСѓСЃРє РїСЂРѕС†РµСЃСЃРѕРІ
    std::cout << "Start" << std::endl;
    sync_send(1);
    sync_send(2);
    sync_send(5);
    sync_send(6);

    int jobs_count = 12;
    int messages_count = jobs_count * 2;
    for (int i = 0; i < messages_count + 1; i++) {
        // Р’С‹РїРѕР»РЅСЏРµРј СЂР°Р±РѕС‚Сѓ - СѓРјРЅРѕР¶Р°РµРј РјР°С‚СЂРёС†С‹
        double t = matmul(a, b, c, SIZE);
        // std::cout << "matmul duration " << t << std::endl;
        matmul_count++;
        total_work_time += t;

        // РћР¶РёРґР°РµРј СЃРѕРѕР±С‰РµРЅРёСЏ РѕС‚ РїСЂРѕС†РµСЃСЃРѕРІ 1 - 7
        notify_receive();
    }

    // РћР¶РёРґР°РЅРёРµ РїРѕСЃР»РµРґРЅРµРіРѕ РїСЂРѕС†РµСЃСЃР°
    sync_recv(1);
    std::cout << "End" << std::endl;

    // РћС‚С‡С‘С‚
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
    // РўРѕС‡РєР° СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё 1
    // 0 -> (1, 2, 5, 6)
    sync_recv(0);
    do_work();

    // РўРѕС‡РєР° СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё 2
    // (1, 5) -> (1, 4, 5)
    sync_send(5);
    sync_recv(5);
    // send self
    sync_send(4);
    do_work();
    
    // РўРѕС‡РєР° СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё 4
    // (1, 2, 4) -> 1
    sync_recv(2);
    sync_recv(4);
    // send self
    do_work();

    // РўРѕС‡РєР° СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё 5
    sync_send(0);
}

void task_2()
{
    // РўРѕС‡РєР° СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё 1
    // 0 -> (1, 2, 5, 6)
    sync_recv(0);
    do_work();

    // РўРѕС‡РєР° СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё 3
    // (2, 5, 6) -> (2, 3, 7)
    sync_recv(5);
    sync_recv(6);
    // send self
    sync_send(3);
    sync_send(7);
    do_work();

    // РўРѕС‡РєР° СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё 4
    // (1, 2, 4) -> 1
    sync_send(1);
}

void task_3()
{
    // РўРѕС‡РєР° СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё 3
    // (2, 5, 6) -> (2, 3, 7)
    sync_recv(2);
    sync_recv(5);
    sync_recv(6);
    do_work();

    // РўРѕС‡РєР° СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё 5
    sync_send(0);
}

void task_4()
{
    // РўРѕС‡РєР° СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё 2
    // (1, 5) -> (1, 4, 5)
    sync_recv(1);
    sync_recv(5);
    do_work();

    // РўРѕС‡РєР° СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё 4
    // (1, 2, 4) -> 1
    sync_send(1);
}

void task_5()
{
    // РўРѕС‡РєР° СЃРёРЅС…СЂРѕРЅРёР·Р°С†Рё 1
    // 0 -> (1, 2, 5, 6)
    sync_recv(0);
    do_work();

    // РўРѕС‡РєР° СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё 2
    // (1, 5) -> (1, 4, 5)
    sync_recv(1);
    sync_send(1);
    sync_send(4);
    do_work();

    // РўРѕС‡РєР° СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё 3
    // (2, 5, 6) -> (2, 3, 7)
    sync_send(2);
    sync_send(3);
    sync_send(7);

    // РўРѕС‡РєР° СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё 6
    // 7 -> 5
    sync_recv(7);
    do_work();

    // РўРѕС‡РєР° СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё 5
    sync_send(0);
}

void task_6()
{
    // РўРѕС‡РєР° СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё 1
    // 0 -> (1, 2, 5, 6)
    sync_recv(0);
    do_work();

    // РўРѕС‡РєР° СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё 3
    // (2, 5, 6) -> (2, 3, 7)
    sync_send(2);
    sync_send(3);
    sync_send(7);
}

void task_7()
{
    // РўРѕС‡РєР° СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё 3
    // (2, 5, 6) -> (2, 3, 7)
    sync_recv(2);
    sync_recv(5);
    sync_recv(6);
    do_work();

    // РўРѕС‡РєР° СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё 6
    // 7 -> 5
    sync_send(5);
}

// РЎРїРёСЃРѕРє Р·Р°РґР°С‡
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

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    t0 = MPI_Wtime();

    // std::cout << "Rank " << rank << " size " << size << std::endl;

    // Р’ Р·Р°РІРёСЃРёРјРѕСЃС‚Рё РѕС‚ СЂР°РЅРєР° (РЅРѕРјРµСЂР° РїСЂРѕС†РµСЃСЃР°) Р·Р°РїСѓСЃРєР°РµРј СЃРѕРѕС‚РІРµС‚СЃС‚РІСѓСЋС‰СѓСЋ Р·Р°РґР°С‡Сѓ
    if (rank < tasks.size()) {
        tasks[rank]();
    }

    MPI_Finalize();
}
