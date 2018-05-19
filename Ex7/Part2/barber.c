#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "defines.h"
#include "queue.h"

sem_t *bsid = SEM_FAILED;
sem_t *wrsid = SEM_FAILED;
sem_t *wrcsid = SEM_FAILED;
int mid = -1;
void *shm_ptr = (void *)-1;

int seats = 0;

void sigterm_handler(int signum) {
    (void)signum;

    if (bsid != SEM_FAILED) {
        sem_close(bsid);
        sem_unlink(barberSemaphorePrefix);
    }

    if (wrsid != SEM_FAILED) {
        sem_close(wrsid);
        sem_unlink(waitingRoomSemaphorePrefix);
    }

    if (wrcsid != SEM_FAILED) {
        sem_close(wrcsid);
        sem_unlink(waitingRoomCheckSemaphorePrefix);
    }

    if (shm_ptr != (void *)-1) {
        munmap(shm_ptr, sizeof(BarberClientInfo_t) + sizeof(QueueInfo_t) +
                            (sizeof(BarberClientInfo_t) * (seats + 1)));
    }

    if (mid != -1) {
        shm_unlink(sharedMemoryName);
    }

    fprintf(stderr, "Closing.\n");

    exit(EXIT_SUCCESS);
}

void atexit_cleanup(void) { sigterm_handler(0); }

void handle_client(BarberClientInfo_t currentClient) {
    struct timespec ts;
    if (kill(currentClient.pid, 0) == 0) {
        sem_t *semServiceBegin = SEM_FAILED;
        sem_t *semServiceEnd = SEM_FAILED;
        char clientSemName[64] = {0};

        sprintf(clientSemName, "%s-%d", clientServiceBeginSemaphorePrefix,
                currentClient.pid);

        semServiceBegin = sem_open(clientSemName, O_RDONLY);

        if (semServiceBegin == SEM_FAILED) {
            EXIT_WITH_MSG(4, "Failed to open client service begin semaphore")
        }

        sprintf(clientSemName, "%s-%d", clientServiceEndSemaphorePrefix,
                currentClient.pid);

        semServiceEnd = sem_open(clientSemName, O_RDONLY);

        if (semServiceEnd == SEM_FAILED) {
            sem_close(semServiceBegin);
            EXIT_WITH_MSG(4, "Failed to open client service begin semaphore")
        }

        clock_gettime(CLOCK_MONOTONIC, &ts);
        printf("[%ld.%lds] Cutting hair of <%d>!\n", ts.tv_sec, ts.tv_nsec,
               currentClient.pid);

        sem_post(semServiceBegin);

        sleep(1);

        clock_gettime(CLOCK_MONOTONIC, &ts);
        printf("[%ld.%lds] Haircut done to <%d>!\n", ts.tv_sec, ts.tv_nsec,
               currentClient.pid);

        sem_post(semServiceEnd);

        sem_close(semServiceBegin);
        sem_close(semServiceEnd);
    } else {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        fprintf(stderr,
                "[%ld.%lds] Client <%d> is dead. Removing from queue.\n",
                ts.tv_sec, ts.tv_nsec, currentClient.pid);
    }
}

void runBarbershop(void) {
    Queue_t queue;
    BarberClientInfo_t currentClient = {0};
    struct timespec ts;

    /*
     * Semaphores initialization
     */
    {
        bsid =
            sem_open(barberSemaphorePrefix, O_CREAT | O_EXCL | O_RDWR, 0660, 0);

        if (bsid == SEM_FAILED) {
            EXIT_WITH_MSG(2, "Failed to create barber semaphore")
        }

        wrsid = sem_open(waitingRoomSemaphorePrefix, O_CREAT | O_EXCL | O_RDWR,
                         0660, 1);

        if (bsid == SEM_FAILED) {
            EXIT_WITH_MSG(2, "Failed to create waiting room semaphore")
        }

        wrcsid = sem_open(waitingRoomCheckSemaphorePrefix,
                          O_CREAT | O_EXCL | O_RDWR, 0660, 1);

        if (bsid == SEM_FAILED) {
            EXIT_WITH_MSG(2, "Failed to create waiting room check semaphore")
        }
    }

    /*
     * Shared memory initialization
     */
    {
        mid = shm_open(sharedMemoryName, O_CREAT | O_EXCL | O_RDWR, 0666);

        if (mid == -1) {
            EXIT_WITH_MSG(2, "Failed to create shared memory");
        }

        if (ftruncate(mid, (sizeof(BarberClientInfo_t) + sizeof(QueueInfo_t) +
                            (sizeof(BarberClientInfo_t) * (seats + 1)))) ==
            -1) {
            EXIT_WITH_MSG(2, "Failed to truncate shared memory");
        }

        shm_ptr = mmap(NULL,
                       sizeof(BarberClientInfo_t) + sizeof(QueueInfo_t) +
                           (sizeof(BarberClientInfo_t) * (seats + 1)),
                       PROT_READ | PROT_WRITE, MAP_SHARED, mid, 0);

        if (shm_ptr == (void *)-1) {
            EXIT_WITH_MSG(2, "Failed to map");
        }

        queue.chair = shm_ptr;
        queue.info = (void *)((char *)shm_ptr + sizeof(BarberClientInfo_t));
        queue.mem = (void *)((char *)shm_ptr + sizeof(BarberClientInfo_t) +
                             sizeof(QueueInfo_t));
        queue.info->capacity = (seats + 1);
        queue.info->element_size = sizeof(BarberClientInfo_t);
        queue.info->elements = queue.info->head = queue.info->tail = 0;
        queue.info->chairOccupied = false;
    }

    clock_gettime(CLOCK_MONOTONIC, &ts);
    printf("[%ld.%lds] Barber starting <%d>\n", ts.tv_sec, ts.tv_nsec,
           getpid());

    while (true) {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        printf("[%ld.%lds] Barber falling asleep.\n", ts.tv_sec, ts.tv_nsec);

        sem_wait(bsid);

        clock_gettime(CLOCK_MONOTONIC, &ts);
        printf("[%ld.%lds] Barber woke up.\n", ts.tv_sec, ts.tv_nsec);

        sem_wait(wrsid);

        while (queue_dequeue(&queue, &currentClient) == true) {
            clock_gettime(CLOCK_MONOTONIC, &ts);
            printf("[%ld.%lds] Getting client\n", ts.tv_sec, ts.tv_nsec);

            handle_client(currentClient);
        }

        sem_post(wrsid);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        EXIT_WITH_MSG(1, "Not enough arguments.")
    }

    char *endptr = NULL;

    seats = strtol(argv[1], &endptr, 10);

    if (endptr == argv[1]) {
        EXIT_WITH_MSG(1, "Invalid arguments.")
    }

    signal(SIGTERM, sigterm_handler);
    signal(SIGINT, sigterm_handler);

    atexit(atexit_cleanup);

    runBarbershop();

    return 0;
}