#define _XOPEN_SOURCE
#define _DEFAULT_SOURCE

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static int pids[10] = {0};
static int currentIdx = 0;
static int processesRequests[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int canRun = 0;

int random(int min_number, int max_number) {
    return (rand() % (max_number + 1 - min_number) + min_number);
}

void slaveHandler(int signum) {
    if(signum == SIGUSR1 || canRun == 0) {
        canRun = 1;
    }
}

void slaveRunner() {
    srand(time(NULL));

    int secs = rand()%10+1;

    sleep(secs);
    
    sigqueue(getppid(), SIGUSR1, (const union sigval)NULL);

    sigset_t sa;
    sigfillset(&sa);
    sigdelset(&sa, SIGINT);
    sigdelset(&sa, SIGUSR1);
    sigsuspend(&sa);

    while(canRun == 0) {}

    sigqueue(getppid(), random(SIGRTMIN, SIGRTMAX), (const union sigval)NULL);

    exit(secs);
}

void masterHandler(int signum, siginfo_t* info, void* _) {
    int pid = info->si_pid;

    printf("PROCESS: %d SIGNAL %d\n", pid, signum);

    int idx = -1;
    for (int i = 0; i < 10; ++i) {
        if (pids[i] == pid) {
            idx = i;
            break;
        }
    }

    if (idx == -1) return;

    if(processesRequests[idx] < 10) {
        processesRequests[idx]++;

        if(processesRequests[idx] == 10) {
            sigqueue(pid, SIGUSR1, (const union sigval)NULL);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Not enouogh arguments.\n");
        return -1;
    }

    int N = itoa(argv[1]);
    int M = itoa(argv[2]);

    if (N <= 0 || M <= 0) {
        fprintf(stderr, "Invalid arguments.\n");
        return -1;
    }

    for (currentIdx = 0; currentIdx < 10; ++currentIdx) {
        pids[currentIdx] = fork();

        if (pids[currentIdx] == 0) {
            slaveRunner();
            exit(0);
        } else if (pids[currentIdx] < 0) {
            fprintf(stderr, "Failed to fork process.\n");
            exit(1);
        }
    }

    masterRunner();

    return 0;
}