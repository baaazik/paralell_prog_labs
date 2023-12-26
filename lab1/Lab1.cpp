п»ї#include <Windows.h>
#include <mpi.h>
#include <iostream>
#include <iomanip>
#include <functional>
#include <vector>

/* РџСЂРѕС†РµСЃСЃ 0 РІС‹РїРѕР»РЅСЏРµС‚ РЅРµРєСѓСЋ СЂР°Р±РѕС‚Сѓ.
 * РџСЂРѕС†РµСЃСЃС‹ 1-N РІС‹РїРѕР»РЅСЏСЋС‚СЃСЏ РІ РїРѕСЂСЏРґРєРµ, РѕРїСЂРµРґРµР»РµРЅРЅС‹Рј РіСЂР°С„РѕРј РІ РёРЅРґРёРІРёРґСѓР°Р»СЊРЅРѕРј
 * РІР°СЂРёР°РЅС‚Рµ.  */

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

// Р¤СѓРЅРєС†РёСЏ РёРјРёС‚РёСЂСѓРµС‚ СЂР°Р±РѕС‚Сѓ РїСЂРѕС†РµСЃСЃР° - Р·Р°РґРµСЂР¶РєР°
void do_work()
{
    Sleep(3000);
}

// Р‘Р»РѕРєРёСЂСѓСЋС‰Р°СЏ СЂС‚РїСЂР°РІРєР° СЃРѕРѕР±С‰РµРЅРёСЏ СЃРёРЅС…СЂРѕРЅРёР·Р°РёС†Рё СѓРєР°Р·Р°РЅРЅРѕРјСѓ РїСЂРѕС†РµСЃСЃСѓ
void sync_send(int dest)
{
    MPI_Send(NULL, 0, MPI_INT, dest, TAG_SYNC, MPI_COMM_WORLD);
}

// РћР¶РёРґР°РЅРёРµ СЃРѕРѕР±С‰РµРЅРёСЏ СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё РѕС‚ СѓРєР°Р·Р°РЅРЅРѕРіРѕ РїСЂРѕС†РµСЃСЃР°
void sync_recv(int src)
{
    MPI_Status status;
    MPI_Recv(NULL, 0, MPI_INT, src, TAG_SYNC, MPI_COMM_WORLD, &status);
}

// Р‘Р»РѕРєРёСЂСѓСЋС‰Р°СЏ РѕС‚РїСЂР°РІРєР° РЅСѓР»РµРІРѕРјСѓ РїСЂРѕС†РµСЃСЃСѓ СѓРІРµРґРѕРјР»РµРЅРёСЏ Рѕ РЅР°С‡Р°Р»Рµ СЂР°Р±РѕС‚С‹
void notify_start(Notification& n)
{
    //Notification n{ start - t0, 0, 0 };
    n.start = MPI_Wtime() - t0;
    n.end = 0;
    n.duration = 0;

    MPI_Send(&n, sizeof(n), MPI_CHAR, 0, TAG_START, MPI_COMM_WORLD);
}

// Р‘Р»РѕРєРёСЂСѓСЋС‰Р°СЏ РѕС‚РїСЂР°РІРєР° РЅСѓР»РµРІРѕРјСѓ РїСЂРѕС†РµСЃСЃСѓ СѓРІРµРґРѕРјР»РµРЅРёСЏ РѕР± РѕРєРѕРЅС‡Р°РЅРёРё СЂР°Р±РѕС‚С‹
void notify_end(Notification& n)
{
    n.end = MPI_Wtime() - t0;
    n.duration = n.end - n.start;

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
            << " started: " << std::fixed << std::setw(6) << std::setprecision(3) << n.start
            << std::endl;
    } else if (status.MPI_TAG == TAG_END) {
        // РџРµС‡Р°С‚СЊ СЃРѕРѕР±С‰РµРЅРёСЏ РѕР± РѕРєРѕРЅС‡Р°РЅРёРё СЂР°Р±РѕС‚С‹
        std::cout
            << "Process " << std::setw(2) << status.MPI_SOURCE
            << " ended:   " << std::fixed << std::setw(6) << std::setprecision(3) << n.end
            << " duration: " << std::fixed <<  std::setw(6) << std::setprecision(3) << n.duration
            << std::endl;
    }
}

void task_0()
{
    std::cout << "Start" << std::endl;
    sync_send(1);
    notify_receive();
    notify_receive();
    notify_receive();
    notify_receive();
    sync_recv(2);
    std::cout << "End\n" << std::endl;
}

void task_1()
{
    Notification n;

    sync_recv(0);
    notify_start(n);
    //std::cout << "Task 1 begin" << std::endl;
    do_work();
    notify_end(n);
    //std::cout << "Task 1 end" << std::endl;
    sync_send(2);
}

void task_2()
{
    Notification n;

    sync_recv(1);
    notify_start(n);
    //std::cout << "Task 2 begin" << std::endl;
    do_work();
    notify_end(n);
    //std::cout << "Task 2 end" << std::endl;
    sync_send(0);
}

// РЎРїРёСЃРѕРє Р·Р°РґР°С‡
std::vector<std::function<void()>> tasks = {
    task_0,
    task_1,
    task_2
};

int main(int argc, char** argv)
{
    int size, rank;

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
