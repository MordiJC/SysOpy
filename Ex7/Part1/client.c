#define _DEFAULT_SOURCE

#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "defines.h"
#include "sem.h"

int getFreeSeats(void) { return 0; }

void sendClientForHaircuts(int haircuts) {
    int sid = semaphore_create(BarberSemaphoresNum);
    struct timespec ts;
    pid_t pid = getpid();

    for (int i = 0; i < haircuts; ++i) {
        if (semaphore_getLocks(sid, semAccessSeats) != 0) {
            semaphore_wait(sid, semAccessSeats);
            semaphore_waitForZero(sid, semAccessSeats);
        }

        // TODO: PRzerobić pod siadanie od razu na krześle, jeżeli nie ma innych
        // klientów
        if (getFreeSeats() > 0) {
            clock_gettime(CLOCK_MONOTONIC, &ts);
            printf("[%ld.%lds] [%d] Taking seat\n", ts.tv_sec, ts.tv_nsec, pid);
            takeSeat();

            semaphore_signal(sid, semCustomersReady);
            semaphore_signal(sid, semAccessSeats);

            if (semaphore_getLocks(sid, semAccessSeats) != 0) {
                semaphore_wait(sid, semAccessSeats);
                semaphore_waitForZero(sid, semAccessSeats);
            }

            clock_gettime(CLOCK_MONOTONIC, &ts);
            printf("[%ld.%lds] [%d] Having haircut done\n", ts.tv_sec,
                   ts.tv_nsec, pid);

            semaphore_waitForZero(sid, semHaircut);

            clock_gettime(CLOCK_MONOTONIC, &ts);
            printf("[%ld.%lds] [%d] Leaving with haircut done\n", ts.tv_sec, ts.tv_nsec,
                   pid);

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
        default: break;
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

    runClients(clients, haircuts);

    return 0;
}