#define _DEFAULT_SOURCE

#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "defines.h"
#include "sem.h"
#include "shm.h"
#include "queue.h"

int sid = -1;
int clientSem = -1;
int mid = -1;
void * shm_ptr = (void*)-1;

void sigterm_handler(int signum) {
    (void)signum;

    if (clientSem != -1) {
        semaphore_remove(clientSem);
    }

    if (shm_ptr != (void *)-1) {
        sharedmem_unmap(shm_ptr);
    }

    exit(EXIT_SUCCESS);
}

void atexit_cleanup(void) { sigterm_handler(0); }

void sendClientForHaircuts(int haircuts) {
    sid = semaphore_create(BarberSemaphoresNum, PROJECT_ID);
    struct timespec ts;
    pid_t pid = getpid();
    BarberChair_t * chair = NULL;
    Queue_t * queue = NULL;

    {
        clientSem = semaphore_create(ClientSemaphoresNum, getpid());

        if (clientSem == -1) {
            EXIT_WITH_MSG(2, "Failed to create semaphore.")
        }
    }

    {
        mid = sharedmem_create(0);

        if (mid == -1) {
            EXIT_WITH_MSG(2, "Failed to create shared memory.");
        }

        shm_ptr = sharedmem_map(mid);

        if (shm_ptr == (void *)-1) {
            EXIT_WITH_MSG(2, "Failed to map.");
        }

        chair = shm_ptr;
        queue = (Queue_t *)((char*)shm_ptr + sizeof(BarberChair_t));
    }

    for (int i = 0; i < haircuts; ++i) {
        semaphore_init(clientSem, semServiceBegin, -1);
        semaphore_init(clientSem, semServiceEnd, -1);

        if (semaphore_getLocks(sid, semChair) != 0) {
            semaphore_wait(sid, semChair);
            semaphore_waitForZero(sid, semChair);
        }

        if(!chair->occupied) {
            // occupy chair (we assume that chair is free if queue is empty)

            clock_gettime(CLOCK_MONOTONIC, &ts);
            printf("[%ld.%lds] [%d] Taking seat and waking barber.\n", ts.tv_sec, ts.tv_nsec, pid);

            chair->clientInfo.pid = pid;
            chair->clientInfo.sid = clientSem;
            chair->occupied = true;

            semaphore_signal(sid, semChair);
        } else {
            semaphore_signal(sid, semChair);
            if (semaphore_getLocks(sid, semWaitingRoom) != 0) {
                semaphore_wait(sid, semWaitingRoom);
                semaphore_waitForZero(sid, semWaitingRoom);
            }
            
            if(queue_isFull(queue)) {
                semaphore_signal(sid, semWaitingRoom);

                clock_gettime(CLOCK_MONOTONIC, &ts);
                printf("[%ld.%lds] [%d] Leaving without haircut.\n", ts.tv_sec,
                    ts.tv_nsec, pid);
                continue; // Continue loop
            }

            BarberClientInfo_t bci = {pid};
            queue_enqueue(queue, &bci);

            semaphore_signal(sid, semWaitingRoom);
        }

        if (semaphore_getLocks(clientSem, semServiceBegin) != 0) {
            semaphore_wait(clientSem, semServiceBegin);
            semaphore_waitForZero(clientSem, semServiceBegin);
        }

        clock_gettime(CLOCK_MONOTONIC, &ts);
        printf("[%ld.%lds] [%d] Having haircut done.\n", ts.tv_sec,
                ts.tv_nsec, pid);

        if (semaphore_getLocks(clientSem, semServiceEnd) != 0) {
            semaphore_wait(clientSem, semServiceEnd);
            semaphore_waitForZero(clientSem, semServiceEnd);
        }

        clock_gettime(CLOCK_MONOTONIC, &ts);
        printf("[%ld.%lds] [%d] Leaving after having haircut done.\n", ts.tv_sec,
                ts.tv_nsec, pid);
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