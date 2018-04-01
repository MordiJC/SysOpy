#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

typedef char** Command;

typedef enum {
    CAPS_OUT = 0u,
    CAPS_SINGLE_WORD,
    CAPS_QUOTED
} CommandArgumentParsingState;

size_t wordLength(const char* str) {
    size_t len = 0;

    while (*(str) != '\0' && *(str) != ' ' && *(str++) != '\t') {
        len++;
    }
    return len;
}

size_t quotedStringLength(const char* str) {
    size_t len = 0;

    if (str[0] == '\"') {
        str++;
    }

    for (; str[len] != '\0'; ++len) {
        if (str[len] == '\"') {
            return len + 2;
        }
    }

    return len;
}
#include <sys/types.h>
#include <sys/wait.h>
size_t argumentLength(const char* line) {
    size_t lineLen = strlen(line);
    CommandArgumentParsingState caps = CAPS_OUT;

    if (lineLen == 0) return 0;

    size_t i = 0;

    if (line[i] == '\"') {
        caps = CAPS_QUOTED;
    } else {
        caps = CAPS_SINGLE_WORD;
    }

    if (caps == CAPS_SINGLE_WORD) {
        return wordLength(line);
    } else if (caps == CAPS_QUOTED) {
        return quotedStringLength(line);
    }

    return 0;
}

size_t countArguments(const char* line) {
    size_t args = 0;

    size_t line_length = strlen(line);

    for (size_t i = 0; i < line_length;) {
        if (line[i] == '\t' || line[i] == ' ') {
            i++;
        } else {
            size_t argLen = argumentLength(&line[i]);
            i += argLen;
            args++;
        }
    }

    return args;
}

char* prepareArgument(char* arg, size_t len) {
    if (arg[0] == '\"') {
        arg += 1;
        len--;
        if (arg[len - 1] == '\"') {
            len--;
        }
    }

    char* outStr = (char*)calloc(len + 1, sizeof(char));

    strncpy(outStr, arg, len);

    return outStr;
}

char** prepareCommand(char* line) {
    size_t line_length = strlen(line);
    size_t argc = countArguments(line);
    char** argv = (char**)calloc(argc + 1, sizeof(char*));

    size_t currentArg = 0;
    for (size_t i = 0; i < line_length;) {
        if (line[i] == '\t' || line[i] == ' ') {
            i++;
        } else {
            size_t argLen = argumentLength(&line[i]);
            argv[currentArg] = prepareArgument(&line[i], argLen);
            i += argLen;
            currentArg++;
        }
    }

    argv[currentArg] = NULL;

    return argv;
}

void releaseCommand(Command argv) {
    Command ptr = argv;
    while((*ptr) != NULL) {
        free(*ptr);
        ptr++;
    }
    free(argv);
}

int executeFile(const char * filePath, char ** error) {
    FILE * inputFile = fopen(filePath, "r");

    if(inputFile == NULL) {
        *error = "Unable to open input file";
        return 0;
    }

    char lineBuffer[1024] = {0};
    int lineNo = 0;

    while(fscanf(inputFile, "%1023[^\n]\n", lineBuffer) != EOF) {
        Command commandArgs = prepareCommand(lineBuffer);
        lineNo++;

        if(commandArgs == NULL || commandArgs[0] == NULL) {
            char num[24];
            sprintf(num, "%d", lineNo);
            char errMsg[25] = "Error at line ";
            strncat(errMsg, num, 23);
            strncat(errMsg, "\n", 1);
            *error = errMsg;
            return 0;
        }

        int procpid = fork();

        if(procpid > 0) {
            wait(NULL);
        } else if(procpid == 0) {
            if(execvp(commandArgs[0], commandArgs) == -1) {
                *error = "Error while executing command";
            } else {
                exit(0);
            }
        }

        releaseCommand(commandArgs);
    }

    fclose(inputFile);

    return -1;
}

int main(int argc, char * argv[]) {
    if(argc < 2) {
        fprintf(stderr, "Not enough arguments\n");
        return 1;
    }

    char * errorMsg = NULL;

    if(!executeFile(argv[1], &errorMsg)){
        fprintf(stderr, "%s\n", errorMsg);
        return 1;
    }

    return 0;
}