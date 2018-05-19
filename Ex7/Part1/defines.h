#ifndef DEFINES_H_
#define DEFINES_H_

#include <sys/types.h>
#include <stdbool.h>

#define PROJECT_ID 420

#define LINE_LEN_MAX 2048

enum BarberSemaphores {
    semBarber = 0u,
    semWaitingRoom = 1u,
    semWaitingRoomCheck = 2u,
    BarberSemaphoresNum,
};

enum ClientSemaphores {
    semServiceBegin = 0u,
    semServiceEnd = 1u,
    ClientSemaphoresNum
};

typedef struct BarberClientInfoStruct {
    pid_t pid;
    int sid;
} BarberClientInfo_t;

#define EXIT_WITH_MSG(code, msg)        \
    {                                   \
        fprintf(stderr, "%s\n", (msg)); \
        exit(code);                     \
    }

#endif
