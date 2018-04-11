#define _XOPEN_SOURCE 500
#define _DEFAULT_SOURCE

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/prctl.h>

static int N = 0;
static int M = 0;
static int M_acc = 0;
static pid_t* pidsArr = NULL;
static int* pidsRequests = NULL;
static int dead = 0;

int canRun = 0;

int randomInt(int min_number, int max_number) {
    return (rand() % (max_number + 1 - min_number) + min_number);
}

void slaveHandler(int signum) {
    if (signum == SIGUSR1 && canRun == 0) {
        fprintf(stderr, "Can Go\n");
        canRun = 1;
    }
}

void slaveRunner(void) {
    srand(time(NULL));

    signal(SIGUSR1, slaveHandler);

    int secs = rand() % 10 + 1;

    fprintf(stderr, "FromSlave: Secs = %d\n", secs);

    sleep(secs);

    fprintf(stderr, "Sending to: %d\n", getppid());

    kill(getppid(), SIGUSR1);

    while (canRun == 0) {
        usleep(50);
    }

    fprintf(stderr, "Can Run %d\n", getpid());

    kill(getppid(), randomInt(SIGRTMIN, SIGRTMAX));

    fprintf(stderr, "Exiting %d\n", secs);

    exit(secs);
}

void master_handler(int signum, siginfo_t* info, void* data) {
    (void)data;
    int pid = info->si_pid;
    union sigval sgv;
    sgv.sival_ptr = NULL;

    printf("PROCESS: %d SIGNAL %d\n", pid, signum);

    int idx = -1;
    for (int i = 0; i < 10; ++i) {
        if (pidsArr[i] == pid) {
            idx = i;
            break;
        }
    }

    if (idx == -1) return;

    if (M_acc >= M) {
        sigqueue(pid, SIGUSR1, sgv);
        return;
    }

    pidsRequests[idx]++;
    M_acc++;

    if (M_acc >= M) {
        for (int i = 0; i < N; ++i) {
            if (pidsRequests[i] != 0) {
                sigqueue(pidsArr[i], SIGUSR1, sgv);
            }
        }
    }
}

void master_sigrt_handler(int signum, siginfo_t* info, void* data) {
    (void)data;
    fprintf(stderr, "PROCESS: %d, SIGNAL: RT %d\n", info->si_pid, signum);
}

void master_sigchld_handler(int signum) {
    (void)signum;
    int status = 0;
    pid_t chldpid = 0;

    fprintf(stderr, "SIGCHLD!\n");

    while (1) {
        chldpid = waitpid(-1, &status, WNOHANG);

        if (chldpid <= 0) {
            return;
        }

        printf("PROCESS: %d, STATUS: %d, RETURN: %d\n", chldpid, status,
               WEXITSTATUS(status));

        dead++;
    }
}

void masterRunner(void) {
    fprintf(stderr, "Starting master %d\n", getpid());

    int pppid;
    while((pppid = waitpid(-1, NULL, 0)) > 0) {

    }
}
int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Not enouogh arguments.\n");
        return -1;
    }

    N = atoi(argv[1]);
    M = atoi(argv[2]);

    if (N <= 0 || M <= 0) {
        fprintf(stderr, "Invalid arguments.\n");
        return -1;
    }

    pidsArr = (pid_t*)calloc(N, sizeof(pid_t));
    pidsRequests = (pid_t*)calloc(N, sizeof(int));

    fprintf(stderr, "N = %d\nM = %d\n", N, M);

    signal(SIGCHLD, master_sigchld_handler);

    struct sigaction sa;
    sa.sa_sigaction = master_sigrt_handler;
    sigemptyset(&sa.sa_mask);

    for (int i = SIGRTMIN; i <= SIGRTMAX; ++i) {
        sigaction(i, &sa, NULL);
    }

    sa.sa_sigaction = master_handler;
    sigaction(SIGUSR1, &sa, NULL);

    for (int i = 0; i < N; ++i) {
        pidsArr[i] = fork();

        if (pidsArr[i] == 0) {
            slaveRunner();
            exit(0);
        } else if (pidsArr[i] < 0) {
            fprintf(stderr, "Failed to fork process.\n");
            exit(1);
        }
    }

    masterRunner();

    free(pidsArr);
    free(pidsRequests);

    fprintf(stderr, "Ending\n");

    return 0;
}