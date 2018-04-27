#define _DEFAULT_SOURCE

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>

#include "qio.h"

#define EXIT_WITH_ERROR(code, msg)      \
    {                                   \
        fprintf(stderr, "%s\n", (msg)); \
        exit((code));                   \
    }

void unregisterClient(int s);

int queueToRemove = -1;
int globalServerQueue = -1;

void remove_queue_at_exit(void) {
    if (queueToRemove != -1) {
        systemv_queue_close(queueToRemove);
    }
}

void sig_int_handler(int signum) {
    (void)signum;
    unregisterClient(globalServerQueue);
    exit(2);
}

void registerClient(int serverQueueId, int clientQueueId, key_t clientKey) {
    char clientKeyStr[LINE_LEN_MAX] = {0};
    const char* err = NULL;
    QueueMessage_t msg;

    sprintf(clientKeyStr, "%d", clientKey);

    systemv_queue_send_message(serverQueueId, LOGIN, getpid(),
                               strlen(clientKeyStr) + 1, clientKeyStr);

    err = systemv_get_error_message();

    if (err != NULL) {
        EXIT_WITH_ERROR(1, err);
    }

    systemv_queue_receive_message(clientQueueId, &msg);

    err = systemv_get_error_message();

    if (err != NULL) {
        EXIT_WITH_ERROR(1, err);
    }

    if (msg.type == CONNECTION_REFUSED) {
        EXIT_WITH_ERROR(1,
                        "Connection refused: too many clients.\nTerminating.");
    }

    if (msg.type != CONNECTION_ACCEPTED) {
        EXIT_WITH_ERROR(1, "Malformed message");
    }
}

void unregisterClient(int serverQueueId) {
    const char* err = NULL;

    systemv_queue_send_message(serverQueueId, LOGOUT, getpid(), 0, NULL);

    err = systemv_get_error_message();

    if (err != NULL) {
        EXIT_WITH_ERROR(1, err);
    }
}

void doMirror(int serverQueueId, int clientQueueId) {
    char contents[LINE_LEN_MAX] = {0};
    const char* err = NULL;
    QueueMessage_t msg;
    char format[256];

    snprintf(format, sizeof(format), "%%%d[^\n]%%*c", LINE_LEN_MAX - 1);

    printf("Enter string to mirror: \n");

    if(scanf(format, contents) < 1) {
        printf("Failed to read command contents.\n");
        scanf("%*c");
        return;
    }

    systemv_queue_send_message(serverQueueId, MIRROR, getpid(),
                               strlen(contents) + 1, contents);

    err = systemv_get_error_message();

    if (err != NULL) {
        EXIT_WITH_ERROR(1, err);
    }

    systemv_queue_receive_message(clientQueueId, &msg);

    err = systemv_get_error_message();

    if (err != NULL) {
        EXIT_WITH_ERROR(1, err);
    }

    printf("Mirrored text: \n%s\n", msg.data);
}

void doCalc(int serverQueueId, int clientQueueId) {
    char contents[LINE_LEN_MAX] = {0};
    const char* err = NULL;
    QueueMessage_t msg;
    char format[256];

    snprintf(format, sizeof(format), "%%%d[^\n]%%*c", LINE_LEN_MAX - 1);

    printf("Enter expression to calculate: \n");

    if(scanf(format, contents) < 1) {
        printf("Failed to read command contents.\n");
        scanf("%*c");
        return;
    }

    systemv_queue_send_message(serverQueueId, CALC, getpid(),
                               strlen(contents) + 1, contents);

    err = systemv_get_error_message();

    if (err != NULL) {
        EXIT_WITH_ERROR(1, err);
    }

    systemv_queue_receive_message(clientQueueId, &msg);

    err = systemv_get_error_message();

    if (err != NULL) {
        EXIT_WITH_ERROR(1, err);
    }

    printf("Result: %s\n", msg.data);
}

void doTime(int serverQueueId, int clientQueueId) {
    const char* err = NULL;
    QueueMessage_t msg;

    systemv_queue_send_message(serverQueueId, TIME, getpid(), 0, NULL);

    err = systemv_get_error_message();

    if (err != NULL) {
        EXIT_WITH_ERROR(1, err);
    }

    systemv_queue_receive_message(clientQueueId, &msg);

    err = systemv_get_error_message();

    if (err != NULL) {
        EXIT_WITH_ERROR(1, err);
    }

    printf("Result: %s\n", msg.data);
}

void doEnd(int serverQueueId) {
    const char* err = NULL;

    systemv_queue_send_message(serverQueueId, END, getpid(), 0, NULL);

    err = systemv_get_error_message();

    if (err != NULL) {
        EXIT_WITH_ERROR(1, err);
    }
}

void mainClientLoop(int serverQueueId, int clientQueueId, key_t clientKey) {
    char command[LINE_LEN_MAX] = {0};
    char format[256];

    int shouldClose = 0;
    int line = 0;

    snprintf(format, sizeof(format), "%%%d[^\n]%%*c", LINE_LEN_MAX - 1);

    registerClient(serverQueueId, clientQueueId, clientKey);

    printf("Enter commands: \n");

    while (!shouldClose) {
        line++;
        printf("%3d> ", line);
        

        if(scanf(format, command) < 1) {
            printf("Failed to read command at line %d\n", line);
            scanf("%*c");
            continue;
        }

        if (strcmp(command, "mirror") == 0) {
            doMirror(serverQueueId, clientQueueId);
        } else if (strcmp(command, "calc") == 0) {
            doCalc(serverQueueId, clientQueueId);
        } else if (strcmp(command, "time") == 0) {
            doTime(serverQueueId, clientQueueId);
        } else if (strcmp(command, "end") == 0) {
            doEnd(serverQueueId);
        } else if (strcmp(command, "quit") == 0 || strcmp(command, "q") == 0) {
            exit(0);
        } else {
            printf("Invalid command at line %d\n", line);
        }
    }
}

int main(void) {
    int serverQueueId = -1;
    int clientQueueId = -1;
    const char* err = NULL;
    key_t queueKey = -1;

    char* HOME_VAR = getenv("HOME");

    if (HOME_VAR == NULL) {
        EXIT_WITH_ERROR(1, "Failed to read $HOME valriable")
    }

    queueKey = ftok(getenv("HOME"), PROJECT_ID);

    if (queueKey == -1) {
        EXIT_WITH_ERROR(1, "Unable to create queue key")
    }

    serverQueueId = systemv_queue_create(queueKey, 0);

    err = systemv_get_error_message();

    if (err != NULL) {
        EXIT_WITH_ERROR(1, err);
    }

    globalServerQueue = serverQueueId;

    queueKey = ftok(HOME_VAR, getpid());
    if (queueKey == -1) {
        EXIT_WITH_ERROR(1, "Unable to create queue key")
    }

    clientQueueId = systemv_queue_create(queueKey, IPC_CREAT | IPC_EXCL | 0666);

    err = systemv_get_error_message();

    if (err != NULL) {
        EXIT_WITH_ERROR(1, err);
    }

    queueToRemove = clientQueueId;

    atexit(remove_queue_at_exit);

    signal(SIGINT, sig_int_handler);

    mainClientLoop(serverQueueId, clientQueueId, queueKey);

    return 0;
}