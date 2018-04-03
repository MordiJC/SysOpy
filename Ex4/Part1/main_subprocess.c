#define _XOPEN_SOURCE
#define _DEFAULT_SOURCE

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

void sigint_handler(int signum) {
    (void)signum;
    printf("%s\n", "Odebrano sygnał SIGINT");
    exit(0);
}

static volatile int programState = 1;

void sigtstp_handler(int signum) {
    (void)signum;
    if (programState == 0) {
        programState = 1;
    } else {
        fprintf(stderr, "%s\n",
               "Oczekuję na CTRL+Z - kontynuacja albo CTR+C - zakonczenie "
               "programu");
        programState = 0;
    }
}

void runDataPrinting(void) {
    int pidn = fork();

    if(pidn > 0) {
        wait(NULL);
    } else if(pidn == 0){
        static char * params[2] = {"date", NULL};
        if (execvp(params[0], params) == -1) {
            fprintf(stderr, "Failed to execute command");
            exit(1);
        }
        exit(0); // For safety reasons
    } else {
        fprintf(stderr, "Failed to fork\n");
        exit(1);
    }
}

void forker(void) {
    int proc_pid = fork();

    fprintf(stderr, "Forked %d\n", proc_pid);

    if(proc_pid < 0) {
        fprintf(stderr, "Failed to fork process.\n");
        exit(1);
    } else if(proc_pid > 0) {
        fprintf(stderr, "Core running\n");
        while(1) {
            if(programState == 0) {
                if(proc_pid != 0) {
                    kill(proc_pid, SIGTERM);
                    proc_pid = 0;
                }
            } else {
                if(proc_pid == 0) {
                    return;
                }
            }
        }
    } else if(proc_pid == 0) {
        signal(SIGINT, SIG_DFL);

        struct sigaction act;
        act.sa_handler = SIG_DFL;
        sigemptyset(&act.sa_mask);
        act.sa_flags = 0;
        sigaction(SIGTSTP, &act, NULL);

        fprintf(stderr, "Child running\n");

        while(1) {
            runDataPrinting();
        }

        exit(0);
    }
}

int main(void) {
    struct sigaction act;

    signal(SIGINT, sigint_handler);

    act.sa_handler = sigtstp_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGTSTP, &act, NULL);

    while(1) {
        forker();
    }

    return 0;
}