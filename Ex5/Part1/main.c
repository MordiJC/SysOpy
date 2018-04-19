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
        } else if(line[i] == '|') {
            i++;
            argv[currentArg] = (char*) calloc(2, sizeof(char));
            strncpy(argv[currentArg], "|", 1);
            currentArg++;
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

#define MAX_COMMANDS_IN_PIPE_LINE 30

Command * parseCommandLine(char * line) {
    char * commands[MAX_COMMANDS_IN_PIPE_LINE];
    int commandNumber = 0;

    while((commands[commandNumber] = strtok((commandNumber ? NULL : line),"|")) != NULL) {
       commandNumber++; 
    }

    Command * subCommands = (Command *)calloc(commandNumber + 1, sizeof(Command));

    for(int i = 0; i < commandNumber; ++i) {
        subCommands[i] = prepareCommand(commands[i]);
    }
    
    subCommands[commandNumber] = NULL;

    return subCommands;
}

void releaseCommand(Command argv) {
    Command ptr = argv;
    while((*ptr) != NULL) {
        free(*ptr);
        ptr++;
    }
    free(argv);
}

void releaseCommandLine(Command * commandLine) {
    Command * ptr = commandLine;
    for(int i = 0; ptr[i] != NULL; ++i) {
        releaseCommand(ptr[i]);
    }
    free(commandLine);
}

int executeFile(const char * filePath, char ** error) {
    FILE * inputFile = fopen(filePath, "r");

    if(inputFile == NULL) {
        *error = "Unable to open input file";
        return 0;
    }

    char lineBuffer[1024] = {0};
    int lineNo = 0;

    int pipes[2][2];

    while(fscanf(inputFile, "%1023[^\n]\n", lineBuffer) != EOF) {
        Command * commandLine = parseCommandLine(lineBuffer);
        lineNo++;

        if(commandLine == NULL) {
            char num[24];
            sprintf(num, "%d", lineNo);
            char errMsg[25] = "Error at line ";
            strncat(errMsg, num, 23);
            strncat(errMsg, "\n", 1);
            *error = errMsg;
            return 0;
        }

        size_t i = 0;
        for(; commandLine[i] != NULL; ++i){

            if(i != 0) {
                close(pipes[i % 2][0]);
                close(pipes[i % 2][1]);
            }

            if(pipe(pipes[i%2]) == -1) {
                char num[24];
                sprintf(num, "%d", lineNo);
                char errMsg[32] = "Error on pipe at line ";
                strncat(errMsg, num, 23);
                strncat(errMsg, "\n", 1);
                *error = errMsg;
                return 0;
            }

            int procpid = fork();

            if(procpid == 0) {
                if(commandLine[i+1] != NULL) {
                    close(pipes[i % 2][0]);
                    if (dup2(pipes[i % 2][1], STDOUT_FILENO) < 0) {
                        exit(EXIT_FAILURE);
                    }
                }

                if (i != 0) {
                    close(pipes[(i + 1) % 2][1]);
                    if (dup2(pipes[(i + 1) % 2][0], STDIN_FILENO) < 0) {
                        close(EXIT_FAILURE);
                    }
                }

                if(execvp(commandLine[i][0], commandLine[i]) == -1) {
                    char num[24];
                    sprintf(num, "%d", lineNo);
                    char errMsg[32] = "Error on execvp at line ";
                    strncat(errMsg, num, 23);
                    strncat(errMsg, "\n", 1);
                    *error = errMsg;
                    return 0;
                }

                exit(EXIT_SUCCESS);
            }

        }

        releaseCommandLine(commandLine);

        close(pipes[i % 2][0]);
        close(pipes[i % 2][1]);
        wait(NULL);
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