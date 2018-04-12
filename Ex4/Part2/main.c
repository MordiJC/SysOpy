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

int random_int(int min_number, int max_number) {
    return (rand() % (max_number + 1 - min_number) + min_number);
}

void slave_handler(int signum) {
    if (signum == SIGUSR1 && canRun == 0) {
        fprintf(stderr, "Can Go\n");
        canRun = 1;
    }
}

void slave_runner(void) {
    srand((time(NULL) + clock()) % getpid());

    signal(SIGUSR1, slave_handler);

    int secs = rand() % 10 + 1;
    union sigval siv;
    siv.sival_int = 0;

    fprintf(stderr, "FromSlave: Secs = %d\n", secs);

    sleep(secs);

    fprintf(stderr, "Sending to: %d\n", getppid());

    sigqueue(getppid(), SIGUSR1, siv);

    while (canRun == 0) {
        usleep(50);
    }

    fprintf(stderr, "Can Run %d\n", getpid());

    sigqueue(getppid(), random_int(SIGRTMIN, SIGRTMAX), siv);

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
    for (int i = 0; i < N; ++i) {
        if (pidsArr[i] == pid) {
            idx = i;
            break;
        }
    }

    if (idx == -1) return;

    if (M_acc >= M) {
        fprintf(stderr, "Allowing process %d to send SIGRTs\n", pid);
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

void master_sigchld_handler(int signum, siginfo_t* info, void* data) {
    (void)signum;
    (void)info;
    (void)data;
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

void sigint_handler(int signum) {
    fprintf(stderr, "Terminating SIGNUM: %d\n", signum);
}

void master_runner(void) {
    fprintf(stderr, "Starting master %d\n", getpid());

    int ppid;
    while((ppid = wait(NULL)) > 0) {
        fprintf(stderr, "[!!!] Process terminated: %d\n", ppid);
    }

    fprintf(stderr, "Finnished\n");
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

    struct sigaction sa;
    sa.sa_sigaction = master_sigchld_handler;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP | SA_SIGINFO;

    if(sigaction(SIGCHLD, &sa, NULL) != 0) {
        fprintf(stderr, "Failed to register SIGCHLD handler\n");
        exit(1);
    }

    sa.sa_sigaction = master_sigrt_handler;
    sigemptyset(&sa.sa_mask);

    for (int i = SIGRTMIN; i <= SIGRTMAX; ++i) {
        if(sigaction(i, &sa, NULL) != 0) {
            fprintf(stderr, "Failed to register SIGRT %d handler\n", i);
            exit(1);
        }
    }

    sa.sa_sigaction = master_handler;
    if(sigaction(SIGUSR1, &sa, NULL) != 0) {
        fprintf(stderr, "Failed to register SIGUSR1 handler\n");
        exit(1);
    }

    for (int i = 0; i < N; ++i) {
        pidsArr[i] = fork();

        if (pidsArr[i] == 0) {
            slave_runner();
            exit(0);
        } else if (pidsArr[i] < 0) {
            fprintf(stderr, "Failed to fork process.\n");
            exit(1);
        } else if(pidsArr[i] > 0) {
            fprintf(stderr, "Child process created: %d\n", pidsArr[i]);
        }
    }

    master_runner();

    free(pidsArr);
    free(pidsRequests);

    fprintf(stderr, "Ending\n");

    return 0;
}