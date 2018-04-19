#define _DEFAULT_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#define LINE_LEN_MAX 1024

int random_int(int min_number, int max_number) {
    return (rand() % (max_number + 1 - min_number) + min_number);
}

int main(int argc, char* argv[]) {

    srand(time(NULL));
    if (argc < 3) {
        fprintf(stderr, "Not enough arguments\n");
        exit(EXIT_FAILURE);
    }

    char line[LINE_LEN_MAX];
    char dateBuf[LINE_LEN_MAX];

    int fifo = open(argv[1], O_WRONLY);

    int N = (int)strtol(argv[2], NULL, 10);

    int myPid = getpid();

    printf("%d\n", myPid);

    for (int i = 0; i < N; ++i) {
        FILE* date = popen("/bin/date", "r");
        fgets(dateBuf, LINE_LEN_MAX, date);
        pclose(date);

        memset(line, 0, LINE_LEN_MAX);
        sprintf(line, "%d %s\n", myPid, dateBuf);
        write(fifo, line, strlen(line));

        sleep(random_int(1, 5));
    }

    close(fifo);

    return 0;
}