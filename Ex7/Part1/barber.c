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
void *shm_ptr = NULL;

void sigterm_handler(int signum) {
    (void)signum;

    if (sid != -1) {
        semaphore_remove(sid);
    }

    if (shm_ptr != (void*)-1) {
        sharedmem_unmap(shm_ptr);
    }

    if (mid != -1) {
        sharedmem_remove(mid);
    }

    exit(EXIT_SUCCESS);
}

void atexit_cleanup(void) { sigterm_handler(0); }

void runBarbershop(int seats) {
    (void)seats;

    struct timespec ts;
    Queue_t *queue = NULL;
    {
        sid = semaphore_create(BarberSemaphoresNum);
        /*
         * [0] => barberReady    = 1
         * [1] => accessSeats    = 0
         * [2] => customersReady = 1
         * [3] => haircut        = 0
         */
        if (sid == -1) {
            EXIT_WITH_MSG(2, "Failed to create semaphore")
        }

        semaphore_init(sid, semBarberReady, 1);
        semaphore_init(sid, semAccessSeats, 0);
        semaphore_init(sid, semCustomersReady, 1);
        semaphore_init(sid, semHaircut, 0);
    }
    {
        mid = sharedmem_create(sizeof(Queue_t) + (sizeof(int) * (seats + 1)));

        if (mid == -1) {
            EXIT_WITH_MSG(2, "Failed to create shared memory");
        }

        shm_ptr = sharedmem_map(mid);

        if (shm_ptr == (void *)-1) {
            EXIT_WITH_MSG(2, "Failed to map");
        }

        queue = shm_ptr;
        queue_init(queue, (void*)((char*)shm_ptr + sizeof(Queue_t)), (seats + 1), sizeof(int));
    }

    while (true) {
        if (semaphore_getLocks(sid, semCustomersReady) != 0) {
            semaphore_wait(sid, semCustomersReady);
            if (semaphore_getLocks(sid, semCustomersReady) != 0) {
                clock_gettime(CLOCK_MONOTONIC, &ts);
                printf("[%ld.%lds] Barber falling asleep\n", ts.tv_sec,
                       ts.tv_nsec);

                semaphore_waitForZero(sid, semCustomersReady);

                clock_gettime(CLOCK_MONOTONIC, &ts);
                printf("[%ld.%lds] Barber woke up\n", ts.tv_sec, ts.tv_nsec);
            }
        }
        if (semaphore_getLocks(sid, semAccessSeats) != 0) {
            semaphore_wait(sid, semAccessSeats);
            semaphore_waitForZero(sid, semAccessSeats);
        }

        // TODO: GET CUSTOMER FROM FIFO

        semaphore_signal(sid, semBarberReady);
        semaphore_signal(sid, semAccessSeats);

        semaphore_signal(sid, semHaircut);

        clock_gettime(CLOCK_MONOTONIC, &ts);
        printf("[%ld.%lds] Cutting hair <PID_HERE>!\n", ts.tv_sec, ts.tv_nsec);

        clock_gettime(CLOCK_MONOTONIC, &ts);
        printf("[%ld.%lds] Haircut done <PID_HERE>!\n", ts.tv_sec, ts.tv_nsec);

        if (semaphore_getLocks(sid, semAccessSeats) != 0) {
            int locksNum = semaphore_getLocks(sid, semHaircut);
            if (locksNum != 0) {
                semaphore_waitNum(sid, semHaircut, locksNum * (-1));
            }

            // TODO: CHECK IF IT WILL WORK
            // semaphore_init(sid, semHaircut, 0);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        EXIT_WITH_MSG(1, "Not enough arguments")
    }

    char *endptr = NULL;

    int seats = strtol(argv[1], &endptr, 10);

    if (endptr == argv[1]) {
        EXIT_WITH_MSG(1, "Invalid arguments")
    }

    signal(SIGTERM, sigterm_handler);

    atexit(atexit_cleanup);

    runBarbershop(seats);

    return 0;
}