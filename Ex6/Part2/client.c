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

char clientPath[LINE_LEN_MAX] = {0};

void unregisterClient(int s);

mqd_t queueToRemove = -1;
mqd_t globalServerQueue = -1;

void remove_queue_at_exit(void) {
    if (queueToRemove != -1) {
        posix_queue_close(queueToRemove);
    }
}

void sig_int_handler(int signum) {
    (void)signum;
    unregisterClient(globalServerQueue);
    exit(2);
}

void registerClient(mqd_t serverQueueId, mqd_t clientQueueId) {
    const char* err = NULL;
    QueueMessage_t msg;

    posix_queue_send_message(serverQueueId, LOGIN, getpid(),
                               strlen(clientPath) + 1, clientPath);

    err = posix_get_error_message();

    if (err != NULL) {
        EXIT_WITH_ERROR(1, err);
    }

    posix_queue_receive_message(clientQueueId, &msg);

    err = posix_get_error_message();

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

void unregisterClient(mqd_t serverQueueId) {
    const char* err = NULL;

    posix_queue_send_message(serverQueueId, LOGOUT, getpid(), 0, NULL);

    err = posix_get_error_message();

    if (err != NULL) {
        EXIT_WITH_ERROR(1, err);
    }
}

void doMirror(mqd_t serverQueueId, mqd_t clientQueueId) {
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

    posix_queue_send_message(serverQueueId, MIRROR, getpid(),
                               strlen(contents) + 1, contents);

    err = posix_get_error_message();

    if (err != NULL) {
        EXIT_WITH_ERROR(1, err);
    }

    posix_queue_receive_message(clientQueueId, &msg);

    err = posix_get_error_message();

    if (err != NULL) {
        EXIT_WITH_ERROR(1, err);
    }

    printf("Mirrored text: \n%s\n", msg.data);
}

void doCalc(mqd_t serverQueueId, mqd_t clientQueueId) {
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

    posix_queue_send_message(serverQueueId, CALC, getpid(),
                               strlen(contents) + 1, contents);

    err = posix_get_error_message();

    if (err != NULL) {
        EXIT_WITH_ERROR(1, err);
    }

    posix_queue_receive_message(clientQueueId, &msg);

    err = posix_get_error_message();

    if (err != NULL) {
        EXIT_WITH_ERROR(1, err);
    }

    printf("Result: %s\n", msg.data);
}

void doTime(mqd_t serverQueueId, mqd_t clientQueueId) {
    const char* err = NULL;
    QueueMessage_t msg;

    posix_queue_send_message(serverQueueId, TIME, getpid(), 0, NULL);

    err = posix_get_error_message();

    if (err != NULL) {
        EXIT_WITH_ERROR(1, err);
    }

    posix_queue_receive_message(clientQueueId, &msg);

    err = posix_get_error_message();

    if (err != NULL) {
        EXIT_WITH_ERROR(1, err);
    }

    printf("Result: %s\n", msg.data);
}

void doEnd(mqd_t serverQueueId) {
    const char* err = NULL;

    posix_queue_send_message(serverQueueId, END, getpid(), 0, NULL);

    err = posix_get_error_message();

    if (err != NULL) {
        EXIT_WITH_ERROR(1, err);
    }
}

void mainClientLoop(mqd_t serverQueueId, mqd_t clientQueueId) {
    char command[LINE_LEN_MAX] = {0};
    char format[256];

    int shouldClose = 0;
    int line = 0;

    snprintf(format, sizeof(format), "%%%d[^\n]%%*c", LINE_LEN_MAX - 1);

    registerClient(serverQueueId, clientQueueId);

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
    mqd_t serverQueueId = -1;
    mqd_t clientQueueId = -1;
    const char* err = NULL;

    serverQueueId = posix_queue_open(serverPath, O_WRONLY);

    err = posix_get_error_message();

    if (err != NULL) {
        EXIT_WITH_ERROR(1, err);
    }

    globalServerQueue = serverQueueId;

    sprintf(clientPath, "/c%d", getpid());

    clientQueueId = posix_queue_create(clientPath, O_RDONLY | O_CREAT | O_EXCL, 0666);

    err = posix_get_error_message();

    if (err != NULL) {
        EXIT_WITH_ERROR(1, err);
    }

    queueToRemove = clientQueueId;

    atexit(remove_queue_at_exit);

    signal(SIGINT, sig_int_handler);

    mainClientLoop(serverQueueId, clientQueueId);

    return 0;
}