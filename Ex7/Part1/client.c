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
void * shm_ptr = -1;

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
    Queue_t * queue = NULL;

    {
        clientSem = semaphore_create(1, getpid());

        if (clientSem == -1) {
            EXIT_WITH_MSG(2, "Failed to create semaphore")
        }
    }

    {
        mid = sharedmem_create(0);

        if (mid == -1) {
            EXIT_WITH_MSG(2, "Failed to create shared memory");
        }

        shm_ptr = sharedmem_map(mid);

        if (shm_ptr == (void *)-1) {
            EXIT_WITH_MSG(2, "Failed to map");
        }

        queue = shm_ptr;
    }

    for (int i = 0; i < haircuts; ++i) {
        semaphore_init(clientSem, 0, 1);

        if (semaphore_getLocks(sid, semAccessSeats) != 0) {
            semaphore_wait(sid, semAccessSeats);
            semaphore_waitForZero(sid, semAccessSeats);
        }

        // TODO: Przerobić pod siadanie od razu na krześle, jeżeli nie ma innych
        // klientów

        if (!queue_isFull(queue)) {
            clock_gettime(CLOCK_MONOTONIC, &ts);
            printf("[%ld.%lds] [%d] Taking seat\n", ts.tv_sec, ts.tv_nsec, pid);
            {
                // Take seat
                
            }

            semaphore_signal(sid, semAccessSeats);
            semaphore_signal(sid, semCustomersReady);

            if (semaphore_getLocks(sid, semAccessSeats) != 0) {
                semaphore_wait(sid, semAccessSeats);
                semaphore_waitForZero(sid, semAccessSeats);
            }

            clock_gettime(CLOCK_MONOTONIC, &ts);
            printf("[%ld.%lds] [%d] Having haircut done\n", ts.tv_sec,
                   ts.tv_nsec, pid);

            semaphore_waitForZero(sid, semHaircut);

            clock_gettime(CLOCK_MONOTONIC, &ts);
            printf("[%ld.%lds] [%d] Leaving with haircut done\n", ts.tv_sec,
                   ts.tv_nsec, pid);

        } else {
            semaphore_signal(sid, semAccessSeats);
            clock_gettime(CLOCK_MONOTONIC, &ts);
            printf("[%ld.%lds] [%d] Leaving without haircut\n", ts.tv_sec,
                   ts.tv_nsec, pid);
        }
    }
}

pid_t createClient(int haircuts) {
    pid_t pid = 0;

    pid = fork();
    switch (pid) {
        case -1:
            EXIT_WITH_MSG(1, "Failed to fork")
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
        EXIT_WITH_MSG(1, "Not enough arguments")
    }

    char *endptr = NULL;

    int clients = strtol(argv[1], &endptr, 10);

    if (endptr == argv[1]) {
        EXIT_WITH_MSG(1, "Invalid arguments")
    }

    int haircuts = strtol(argv[2], &endptr, 10);

    if (endptr == argv[2]) {
        EXIT_WITH_MSG(1, "Invalid arguments")
    }

    assert(clients > 0);
    assert(haircuts > 0);

    signal(SIGTERM, sigterm_handler);

    atexit(atexit_cleanup);

    runClients(clients, haircuts);

    return 0;
}