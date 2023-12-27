#ifndef __COMMON_H__
#define __COMMON_H__

// Теги сообщений MPI
#define TAG_SYNC  1
#define TAG_START 2
#define TAG_END   3

 // Структура, отправляемая i-м процессом 0-му процессу для оповещния об
 // окончании работы
struct Notification {
    double start;
    double end;
    double duration;
};

#endif