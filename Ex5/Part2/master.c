#define _DEFAULT_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>

#define LINE_LEN_MAX 1024

int main(int argc, char * argv[]) {

    if(argc < 2) {
        fprintf(stderr, "Not enough arguments\n");
        exit(EXIT_FAILURE);
    }

    mkfifo(argv[1], S_IWUSR | S_IRUSR);

    char line[LINE_LEN_MAX];

    FILE * fifo = fopen(argv[1], "r");

    while(fgets(line, LINE_LEN_MAX, fifo) != NULL) {
        write(STDOUT_FILENO, line, strlen(line));
    }

    fclose(fifo);

    return 0;
}