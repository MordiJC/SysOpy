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

void barber_sleep(int semid, int semIdx) {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    printf("[%ld.%lds] Barber falling asleep.\n", ts.tv_sec, ts.tv_nsec);

    semaphore_wait(semid, semIdx);
    semaphore_waitForZero(semid, semIdx);

    clock_gettime(CLOCK_MONOTONIC, &ts);
    printf("[%ld.%lds] Barber woke up.\n", ts.tv_sec, ts.tv_nsec);
}

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
    (void)seats;

    Queue_t queue;
    BarberChair_t *chair = NULL;
    BarberClientInfo_t currentClient = {0};

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

        semaphore_init(sid, semChair, 1);
        semaphore_init(sid, semWaitingRoom, 0);
    }

    /*
     * Shared memory initialization
     */
    {
        mid = sharedmem_create(sizeof(BarberChair_t) + sizeof(QueueInfo_t) +
                               (sizeof(BarberClientInfo_t) * (seats + 1)));

        if (mid == -1) {
            EXIT_WITH_MSG(2, "Failed to create shared memory");
        }

        shm_ptr = sharedmem_map(mid);

        if (shm_ptr == (void *)-1) {
            EXIT_WITH_MSG(2, "Failed to map");
        }

        chair = shm_ptr;
        chair->occupied = false;

        queue.info = (void *)((char *)shm_ptr + sizeof(BarberChair_t));
        queue.mem = (void *)((char *)shm_ptr + sizeof(BarberChair_t) +
                             sizeof(QueueInfo_t));
        queue.info->capacity = (seats + 1);
        queue.info->element_size = sizeof(BarberClientInfo_t);
        queue.info->elements = queue.info->head = queue.info->tail = 0;
    }

    printf("[INFO] Barber starting\n");

    while (true) {
        barber_sleep(sid, semChair);  // Sleep till chair will be occupied

        if (chair->occupied == true) {
            currentClient = chair->clientInfo;
            // Handle client on the chair
            printf("Getting client from chair.\n");
            handle_client(currentClient);

        } else {
            if (semaphore_getLocks(sid, semChair) < 0) {
                semaphore_signal(sid, semChair);
            }
            printf("[INFO] Barber looping\n");
            sleep(1);
            continue;
        }

        // Now check queue.
        printf("Checking queue.\n");

        if (semaphore_getLocks(sid, semWaitingRoom) != 0) {
            semaphore_wait(sid, semWaitingRoom);
            semaphore_waitForZero(sid, semWaitingRoom);
        }

        while (queue_dequeue(&queue, &currentClient)) {
            handle_client(currentClient);
        }

        chair->occupied = false;
        if (semaphore_getLocks(sid, semChair) < 0) {
            semaphore_signal(sid, semChair);
        }
        if (semaphore_getLocks(sid, semWaitingRoom) < 0) {
            semaphore_signal(sid, semWaitingRoom);
        }
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