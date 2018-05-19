#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "defines.h"
#include "queue.h"
#include "sem.h"
#include "shm.h"

int sid = -1;
int mid = -1;
void *shm_ptr = (void *)-1;

void sigterm_handler(int signum) {
    (void)signum;

    if (sid != -1) {
        semaphore_remove(sid);
    }

    if (shm_ptr != (void *)-1) {
        sharedmem_unmap(shm_ptr);
    }

    if (mid != -1) {
        sharedmem_remove(mid);
    }

    exit(EXIT_SUCCESS);
}

void atexit_cleanup(void) { sigterm_handler(0); }

void handle_client(BarberClientInfo_t currentClient) {
    struct timespec ts;
    if (kill(currentClient.pid, 0) == 0) {
        int clientSemaphore =
            currentClient.sid;  // [0] = ToClient [1] = To Barber

        clock_gettime(CLOCK_MONOTONIC, &ts);
        printf("[%ld.%lds] Cutting hair of <%d>!\n", ts.tv_sec, ts.tv_nsec,
               currentClient.pid);

        semaphore_signal(clientSemaphore, semServiceBegin);

        sleep(1);

        clock_gettime(CLOCK_MONOTONIC, &ts);
        printf("[%ld.%lds] Haircut done to <%d>!\n", ts.tv_sec, ts.tv_nsec,
               currentClient.pid);

        semaphore_signal(clientSemaphore, semServiceEnd);
    } else {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        fprintf(stderr,
                "[%ld.%lds] Client <%d> is dead. Removing from queue.\n",
                ts.tv_sec, ts.tv_nsec, currentClient.pid);
    }
}

void runBarbershop(int seats) {
    Queue_t queue;
    BarberClientInfo_t currentClient = {0, 0};
    struct timespec ts;

    /*
     * Semaphores initialization
     */
    {
        sid = semaphore_create(BarberSemaphoresNum, PROJECT_ID);
        /*
         * semChair = 1,
         * semWaitingRoom = 1,
         */
        if (sid == -1) {
            EXIT_WITH_MSG(2, "Failed to create semaphore")
        }

        semaphore_init(sid, semBarber, 0);
        semaphore_init(sid, semWaitingRoom, 1);
        semaphore_init(sid, semWaitingRoomCheck, 1);
    }

    /*
     * Shared memory initialization
     */
    {
        mid =
            sharedmem_create(sizeof(BarberClientInfo_t) + sizeof(QueueInfo_t) +
                             (sizeof(BarberClientInfo_t) * (seats + 1)));

        if (mid == -1) {
            EXIT_WITH_MSG(2, "Failed to create shared memory");
        }

        shm_ptr = sharedmem_map(mid);

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

        semaphore_wait(sid, semBarber);

        clock_gettime(CLOCK_MONOTONIC, &ts);
        printf("[%ld.%lds] Barber woke up.\n", ts.tv_sec, ts.tv_nsec);

        semaphore_wait(sid, semWaitingRoom);

        while (queue_dequeue(&queue, &currentClient) == true) {
            clock_gettime(CLOCK_MONOTONIC, &ts);
            printf("[%ld.%lds] Getting client\n", ts.tv_sec, ts.tv_nsec);

            handle_client(currentClient);
        }

        semaphore_signal(sid, semWaitingRoom);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        EXIT_WITH_MSG(1, "Not enough arguments.")
    }

    char *endptr = NULL;

    int seats = strtol(argv[1], &endptr, 10);

    if (endptr == argv[1]) {
        EXIT_WITH_MSG(1, "Invalid arguments.")
    }

    signal(SIGTERM, sigterm_handler);
    signal(SIGINT, sigterm_handler);

    atexit(atexit_cleanup);

    runBarbershop(seats);

    return 0;
}