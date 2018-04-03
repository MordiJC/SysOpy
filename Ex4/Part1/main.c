#define _XOPEN_SOURCE
#define _DEFAULT_SOURCE

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void sigint_handler(int signum) {
    (void)signum;
    printf("%s\n", "Odebrano sygnał SIGINT");
    exit(0);
}

static volatile int block = 0;

void sigtstp_handler(int signum) {
    (void)signum;
    if (block == 0) {
        block = 1;
        printf("%s\n",
               "Oczekuję na CTRL+Z - kontynuacja albo CTR+C - zakonczenie "
               "programu");

        sigset_t sa;
        sigfillset(&sa);
        sigdelset(&sa, SIGTSTP);
        sigdelset(&sa, SIGINT);
        sigsuspend(&sa);
    } else {
        block = 0;
    }
}

int main(void) {
    char timeStr[16] = {0};
    time_t rawtime;
    struct tm* timeinfo;
    struct sigaction act;

    signal(SIGINT, sigint_handler);

    act.sa_handler = sigtstp_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGTSTP, &act, NULL);

    while (1) {
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(timeStr, 16, "%T", timeinfo);

        printf("%s\n", timeStr);
    }

    return 0;
}