#define _DEFAULT_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>

#include "qio.h"

#define PROJECT_ID 420

#define EXIT_WITH_ERROR(code, msg)    \
    {                                 \
        fprintf(stderr, "%s\n", msg); \
        exit(code);                   \
    }

int shouldClose = 0;

void loginClient(QueueMessageHead_t* msg_head,
                 QueueMessageBodyUnit_t* msg_body) {
    (void)msg_head;
    (void)msg_body;
}

void executeMessage(QueueMessageHead_t* msg_head,
                    QueueMessageBodyUnit_t* msg_body) {
    if (msg_head == NULL) return;

    switch (msg_head->type) {
        case LOGIN:
            loginClient(msg_head, msg_body);
            break;
        case MIRROR:
        case CALC:
        case TIME:
            break;
        case END:
            shouldClose = 1;
            break;
        default:
            break;
    }
}

void mainServerLoop(int queueId) {
    QueueMessageHead_t messageHead;
    QueueMessageBodyUnit_t bodyBuffer[MSG_MAX_BODY_SIZE];

    struct msqid_ds state;

    while (shouldClose == 0) {
        if (msgctl(queueId, IPC_STAT, &state) == -1) {
            EXIT_WITH_ERROR(1, "Failed to get current state of public queue");
        }

        // if(state.msg_qnum == 0) break; /* No messages to process. Quitting */

        if (msgrcv(queueId, &messageHead, QueueMessageHeadSize, 0, 0) < 0) {
            EXIT_WITH_ERROR(1, "Failed to receive message head");
        }

        if (msgrcv(queueId, bodyBuffer,
                   messageHead.bodySize * sizeof(QueueMessageBodyUnit_t), 0,
                   0) < 0) {
            EXIT_WITH_ERROR(1, "Failed to receive message body");
        }

        executeMessage(&messageHead, bodyBuffer);
    }
}

int createQueue(key_t key) {
    int qid = msgget(key, IPC_CREAT | IPC_EXCL | 0666);

    if (qid == -1) {
        if (errno == EEXIST) {
            EXIT_WITH_ERROR(1, "Queue already exists");
        } else {
            EXIT_WITH_ERROR(1, "Failed to create queue");
        }
    }

    return qid;
}

int queueToRemove = -1;

void remove_queue_at_exit() { systemv_queue_close(queueToRemove); }

int main(void) {
    char* HOME_VAR = getenv("HOME");

    if (HOME_VAR == NULL) {
        EXIT_WITH_ERROR(1, "Failed to read $HOME valriable")
    }

    key_t queueKey = ftok(getenv("HOME"), PROJECT_ID);

    if (queueKey == -1) {
        EXIT_WITH_ERROR(1, "Unable to create queue key")
    }

    int queueId = createQueue(queueKey);
    queueToRemove = queueId;

    atexit(remove_queue_at_exit);

    mainServerLoop(queueId);

    return 0;
}