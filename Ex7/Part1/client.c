#define _DEFAULT_SOURCE

#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "defines.h"
#include "queue.h"
#include "sem.h"
#include "shm.h"

int sid = -1;
int clientSem = -1;
int mid = -1;
void *shm_ptr = (void *)-1;

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

void atexit_cleanup(void) {
    fprintf(stderr, "Exitting\n");
    sigterm_handler(0);
}

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

void sendClientForHaircuts(int haircuts) {
    const char *color = colors[getpid() % 6];
    struct timespec ts;
    pid_t pid = getpid();
    BarberChair_t *chair = NULL;
    Queue_t queue;

    printf("%s[%d] Client started.\033[0m\n", color, getpid());
    sid = semaphore_create(BarberSemaphoresNum, PROJECT_ID);

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

        queue.info = (void *)((char *)shm_ptr + sizeof(BarberChair_t));
        queue.mem = (void *)((char *)shm_ptr + sizeof(BarberChair_t) +
                             sizeof(QueueInfo_t));
    }

    for (int i = 0; i < haircuts; ++i) {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        printf("%s[%ld.%lds] [%d] Haircut no. %d\033[0m\n", color, ts.tv_sec,
               ts.tv_nsec, pid, i);

        semaphore_init(clientSem, semServiceBegin, -1);
        semaphore_init(clientSem, semServiceEnd, -1);

        clock_gettime(CLOCK_MONOTONIC, &ts);
        fprintf(stderr, "%s[%ld.%lds] [%d] Chair semaphore state: %d\033[0m\n",
                color, ts.tv_sec, ts.tv_nsec, pid,
                semaphore_getLocks(sid, semChair));

        // if (semaphore_getLocks(sid, semChair) != 0) {
        // clock_gettime(CLOCK_MONOTONIC, &ts);
        // fprintf(stderr, "[%ld.%lds] [%d] Waiting for chair to free\n",
        //         ts.tv_sec, ts.tv_nsec, pid);
        // semaphore_wait(sid, semChair);
        // }
        semaphore_wait(sid, semChairWrite);
        if (chair->occupied == false) {

            clock_gettime(CLOCK_MONOTONIC, &ts);
            printf("%s[%ld.%lds] [%d] Taking seat and waking barber.\033[0m\n",
                   color, ts.tv_sec, ts.tv_nsec, pid);

            chair->clientInfo.pid = pid;
            chair->clientInfo.sid = clientSem;
            chair->occupied = true;
            semaphore_signal(sid, semChair);
        } else {
            if (semaphore_getLocks(sid, semWaitingRoom) != 0) {
                clock_gettime(CLOCK_MONOTONIC, &ts);
                printf(
                    "%s[%ld.%lds] [%d] Waiting for access to waiting "
                    "room.\033[0m\n",
                    color, ts.tv_sec, ts.tv_nsec, pid);
                semaphore_wait(sid, semWaitingRoom);
            }

            if (queue_isFull(&queue)) {
                semaphore_signal(sid, semWaitingRoom);

                clock_gettime(CLOCK_MONOTONIC, &ts);
                printf("%s[%ld.%lds] [%d] Leaving without haircut.\033[0m\n",
                       color, ts.tv_sec, ts.tv_nsec, pid);
                continue;  // Continue loop
            }

            clock_gettime(CLOCK_MONOTONIC, &ts);
            printf("%s[%ld.%lds] [%d] Taking seat in waiting room.\033[0m\n",
                   color, ts.tv_sec, ts.tv_nsec, pid);

            BarberClientInfo_t bci;
            bci.pid = pid;
            bci.sid = clientSem;
            queue_enqueue(&queue, &bci);

            semaphore_signal(sid, semWaitingRoom);
        }

        semaphore_signal(sid, semChairWrite);

        clock_gettime(CLOCK_MONOTONIC, &ts);
        printf("%s[%ld.%lds] [%d] Waiting for haircut.\033[0m\n", color,
               ts.tv_sec, ts.tv_nsec, pid);

        semaphore_wait(clientSem, semServiceBegin);

        clock_gettime(CLOCK_MONOTONIC, &ts);
        printf("%s[%ld.%lds] [%d] Having haircut done.\033[0m\n", color,
               ts.tv_sec, ts.tv_nsec, pid);

        semaphore_wait(clientSem, semServiceEnd);

        clock_gettime(CLOCK_MONOTONIC, &ts);
        printf("%s[%ld.%lds] [%d] Leaving after having haircut done.\033[0m\n",
               color, ts.tv_sec, ts.tv_nsec, pid);
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