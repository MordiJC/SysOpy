#ifndef DEFINES_H_
#define DEFINES_H_

#include <sys/types.h>
#include <stdbool.h>

#define PROJECT_ID 420

#define LINE_LEN_MAX 2048

typedef struct BarberClientInfoStruct {
    pid_t pid;
} BarberClientInfo_t;

static const char * const barberSemaphorePrefix = "barber-semBarber";
static const char * const waitingRoomSemaphorePrefix = "barber-semWaitingRoom";
static const char * const waitingRoomCheckSemaphorePrefix = "barber-semWaitingRoomCheck";

static const char * const clientServiceBeginSemaphorePrefix = "client-semServiceBegin";
static const char * const clientServiceEndSemaphorePrefix = "client-semServiceEnd";

static const char * const sharedMemoryName = "barber-shm";

#define EXIT_WITH_MSG(code, msg)        \
    {                                   \
        fprintf(stderr, "%s\n", (msg)); \
        exit(code);                     \
    }

#endif
