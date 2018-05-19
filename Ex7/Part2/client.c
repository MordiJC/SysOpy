#define _DEFAULT_SOURCE

#include <assert.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "defines.h"
#include "queue.h"

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[94m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

static const char *const colors[6] = {
    ANSI_COLOR_RED,  ANSI_COLOR_GREEN,   ANSI_COLOR_YELLOW,
    ANSI_COLOR_BLUE, ANSI_COLOR_MAGENTA, ANSI_COLOR_CYAN,
};

sem_t *bsid = SEM_FAILED;
sem_t *wrsid = SEM_FAILED;
sem_t *wrcsid = SEM_FAILED;

sem_t *semClientServiceBegin = SEM_FAILED;
sem_t *semClientServiceEnd = SEM_FAILED;

int mid = -1;
void *shm_ptr = (void *)-1;

int seats = 0;

void sigterm_handler(int signum) {
    (void)signum;

    if (bsid != SEM_FAILED) {
        sem_close(bsid);
    }

    if (wrsid != SEM_FAILED) {
        sem_close(wrsid);
    }

    if (wrcsid != SEM_FAILED) {
        sem_close(wrcsid);
    }

    if (semClientServiceBegin != SEM_FAILED) {
        sem_close(semClientServiceBegin);
        char clientSemName[64] = {0};
        sprintf(clientSemName, "%s-%d", clientServiceBeginSemaphorePrefix,
                getpid());
        sem_unlink(clientSemName);
    }

    if (semClientServiceEnd != SEM_FAILED) {
        sem_close(semClientServiceEnd);
        char clientSemName[64] = {0};
        sprintf(clientSemName, "%s-%d", clientServiceEndSemaphorePrefix,
                getpid());
        sem_unlink(clientSemName);
    }

    if (shm_ptr != (void *)-1) {
        munmap(shm_ptr, sizeof(BarberClientInfo_t) + sizeof(QueueInfo_t) +
                            (sizeof(BarberClientInfo_t) * (seats + 1)));
    }

    exit(EXIT_SUCCESS);
}

void atexit_cleanup(void) {
    const char *color = colors[getpid() % 6];
    struct timespec ts;
    // fprintf(stderr, "%dExitting\033[0m\n", color);
    clock_gettime(CLOCK_MONOTONIC, &ts);
    fprintf(stderr, "%s[%ld.%lds] [%d] Client exitting.\033[0m\n", color,
            ts.tv_sec, ts.tv_nsec, getpid());

    sigterm_handler(0);
}

void sendClientForHaircuts(int haircuts) {
    const char *color = colors[getpid() % 6];
    struct timespec ts;
    pid_t pid = getpid();
    Queue_t queue;
    BarberClientInfo_t bci;
    bool tookSeat = false;

    {
        char clientSemName[64] = {0};

        bsid = sem_open(barberSemaphorePrefix, O_RDWR);

        if (bsid == SEM_FAILED) {
            EXIT_WITH_MSG(2, "Failed to open barber semaphore")
        }

        wrsid = sem_open(waitingRoomSemaphorePrefix, O_RDWR);

        if (bsid == SEM_FAILED) {
            EXIT_WITH_MSG(2, "Failed to open waiting room semaphore")
        }

        wrcsid = sem_open(waitingRoomCheckSemaphorePrefix, O_RDWR);

        if (bsid == SEM_FAILED) {
            EXIT_WITH_MSG(2, "Failed to open waiting room check semaphore")
        }

        sprintf(clientSemName, "%s-%d", clientServiceBeginSemaphorePrefix, pid);
        semClientServiceBegin =
            sem_open(clientSemName, O_CREAT | O_EXCL | O_RDWR, 0660, 0);

        if (semClientServiceBegin == SEM_FAILED) {
            EXIT_WITH_MSG(2, "Failed to create client service begin semaphore")
        }

        sprintf(clientSemName, "%s-%d", clientServiceEndSemaphorePrefix, pid);
        semClientServiceEnd =
            sem_open(clientSemName, O_CREAT | O_EXCL | O_RDWR, 0660, 0);

        if (semClientServiceEnd == SEM_FAILED) {
            EXIT_WITH_MSG(2, "Failed to create client service begin semaphore")
        }
    }

    {
        mid = shm_open(sharedMemoryName, O_RDWR, 0666);

        if (mid == -1) {
            EXIT_WITH_MSG(2, "Failed to create shared memory.");
        }

        shm_ptr = mmap(NULL,
                       sizeof(BarberClientInfo_t) + sizeof(QueueInfo_t) +
                           (sizeof(BarberClientInfo_t) * (seats + 1)),
                       PROT_READ | PROT_WRITE, MAP_SHARED, mid, 0);

        if (shm_ptr == (void *)-1) {
            EXIT_WITH_MSG(2, "Failed to map.");
        }

        queue.chair = shm_ptr;
        queue.info = (void *)((char *)shm_ptr + sizeof(BarberClientInfo_t));
        queue.mem = (void *)((char *)shm_ptr + sizeof(BarberClientInfo_t) +
                             sizeof(QueueInfo_t));

        seats = queue.info->capacity;
    }

    clock_gettime(CLOCK_MONOTONIC, &ts);
    printf("%s[%ld.%lds] [%d] Client started.\033[0m\n", color, ts.tv_sec,
           ts.tv_nsec, getpid());

    for (int i = 0; i < haircuts; ++i) {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        printf("%s[%ld.%lds] [%d] Haircut no. %d\033[0m\n", color, ts.tv_sec,
               ts.tv_nsec, pid, i);

        sem_wait(wrcsid);
        sem_wait(wrsid);

        bci.pid = pid;
        tookSeat = false;

        if (queue_enqueue(&queue, &bci) == false) {
            clock_gettime(CLOCK_MONOTONIC, &ts);
            printf("%s[%ld.%lds] [%d] Leaving without haircut.\033[0m\n", color,
                   ts.tv_sec, ts.tv_nsec, pid);
        } else {
            clock_gettime(CLOCK_MONOTONIC, &ts);
            printf("%s[%ld.%lds] [%d] Taking seat in barber shop.\033[0m\n",
                   color, ts.tv_sec, ts.tv_nsec, pid);

            tookSeat = true;
            sem_post(bsid);
        }

        sem_post(wrsid);
        sem_post(wrcsid);

        if (tookSeat) {
            clock_gettime(CLOCK_MONOTONIC, &ts);
            printf("%s[%ld.%lds] [%d] Waiting for haircut.\033[0m\n", color,
                   ts.tv_sec, ts.tv_nsec, pid);

            sem_wait(semClientServiceBegin);

            clock_gettime(CLOCK_MONOTONIC, &ts);
            printf("%s[%ld.%lds] [%d] Having haircut done.\033[0m\n", color,
                   ts.tv_sec, ts.tv_nsec, pid);

            sem_wait(semClientServiceEnd);

            clock_gettime(CLOCK_MONOTONIC, &ts);
            printf(
                "%s[%ld.%lds] [%d] Leaving after having haircut done.\033[0m\n",
                color, ts.tv_sec, ts.tv_nsec, pid);
        }
    }
}

pid_t createClient(int haircuts) {
    pid_t pid = 0;

    pid = fork();
    switch (pid) {
        case -1:
            EXIT_WITH_MSG(1, "Failed to fork.")
            break;
        case 0:
            sendClientForHaircuts(haircuts);
            exit(EXIT_SUCCESS);
            break;
        default:
            break;
    }

    return pid;
}

void runClients(int clients, int haircuts) {
    for (int i = 1; i < clients; ++i) {
        createClient(haircuts);
    }
    sendClientForHaircuts(haircuts);
    int status;
    while (waitpid(0, &status, 0) != -1) {
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        EXIT_WITH_MSG(1, "Not enough arguments.")
    }

    char *endptr = NULL;

    int clients = strtol(argv[1], &endptr, 10);

    if (endptr == argv[1]) {
        EXIT_WITH_MSG(1, "Invalid arguments.")
    }

    int haircuts = strtol(argv[2], &endptr, 10);

    if (endptr == argv[2]) {
        EXIT_WITH_MSG(1, "Invalid arguments.")
    }

    assert(clients > 0);
    assert(haircuts > 0);

    signal(SIGTERM, sigterm_handler);
    signal(SIGINT, sigterm_handler);

    atexit(atexit_cleanup);

    runClients(clients, haircuts);

    return 0;
}