#define _XOPEN_SOURCE 500
#define _DEFAULT_SOURCE

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <unistd.h>

/* MASTER CODE */
int L = 0;
int Type = 0;
int pid = 0;

int canSend = 0;
int received = 0;

void master_sigusr1_sirtmin_handler(int signum, siginfo_t* info, void* data) {
    (void)data;
    if (info->si_pid == pid) {
        printf("Master process received %s from %d\n", (signum == SIGUSR1 ? "SIGUSR1": "SIGRTMIN"), pid);
        canSend = 1;
        received++;
    }
}

void master_sigchld_handler(int signum, siginfo_t* info, void* data) {
    (void) signum;
    (void) data;
    if (info->si_pid != pid) {
        return;
    }

    waitpid(pid, NULL, 0);
    printf("Child process exited\n");
    exit(0);
}

void master_send_end_signal(void) {
    if (Type == 3) {
        kill(pid, SIGRTMAX);
    } else {
        kill(pid, SIGUSR2);
    }
}

void master_sigint_handler(int signum) {
    (void) signum;
    printf("\nTerminating. Killing children\n");
    master_send_end_signal();
    exit(0);
}

void master_runner(void) {
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = master_sigusr1_sirtmin_handler;
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGUSR1, &sa, NULL) < 0) {
        fprintf(stderr, "Failed to register SIGUSR1 handler\n");
        exit(1);
    }

    if (sigaction(SIGRTMIN, &sa, NULL) < 0) {
        fprintf(stderr, "Failed to register SIGUSR1 handler\n");
        exit(1);
    }

    sa.sa_sigaction = master_sigchld_handler;
    if (sigaction(SIGCHLD, &sa, NULL) < 0) {
        fprintf(stderr, "Failed to register SIGCHLD handler\n");
        exit(1);
    }

    if (signal(SIGINT, master_sigint_handler) == SIG_ERR) {
        fprintf(stderr, "Failed to register SIGINT handler\n");
        exit(1);
    }

    for (int i = 0; i < L; ++i) {
        printf("Sending signal %s to child process: PID %d\n",
               (Type < 3 ? "SIGUSR1" : "SIGRT"), pid);

        if (Type == 1) {
            kill(pid, SIGUSR1);
        } else if (Type == 2) {
            canSend = 0;

            kill(pid, SIGUSR1);

            if ((i + 1) < L) {
                while (canSend == 0) {
                    usleep(10);
                }
            }

        } else if (Type == 3) {
            union sigval siv;
            siv.sival_int = 0;
            sigqueue(pid, SIGRTMIN, siv);
        }
    }

    printf(
        "-------------\nSend %d signals\nReceived %d signals from child "
        "process PID %d\n",
        L, received, pid);

    master_send_end_signal();

    while (wait(NULL) > 0) {
        usleep(100);
    }
}

/* CHILD CODE */
int ppid;
int sigcount = 0;

void child_sigusr1_handler(int sigint) {
    (void) sigint;
    printf("Child got SIGUSR1 signal from parent process.\n");
    sigcount++;
    if(Type < 3){
        kill(ppid, SIGUSR1);
    } else {
        union sigval siv;
        siv.sival_int = 0;
        sigqueue(ppid, SIGRTMIN, siv);
    }
}

void child_sigusr2_handler(int sigint) {
    (void) sigint;
    printf(
        "Child got %d signald from parent process.\nChild process terminating "
        "because of terminating signal from parent process.\n",
        sigcount);
    exit(0);
}

void child_runner(void) {
    fprintf(stderr, "Running child proc\n");
    ppid = getppid();
    signal(SIGUSR1, child_sigusr1_handler);
    signal(SIGUSR2, child_sigusr2_handler);

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGRTMAX);
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGRTMIN, &sa, NULL) < 0 || sigaction(SIGRTMAX, &sa, NULL)) {
        fprintf(stderr, "Unable to register sigrt handlers.\n");
        exit(1);
    }

    while (1) {usleep(10);}
}

/* SHARED CODE */
int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Not enough arguments.\n");
        return -1;
    }

    L = atoi(argv[1]);
    Type = atoi(argv[2]);

    pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Failed to create subprocess\n");
        exit(1);
    } else if (pid > 0) {
        sleep(1);
        master_runner();
    } else if(pid == 0) {
        child_runner();
    }

    return 0;
}