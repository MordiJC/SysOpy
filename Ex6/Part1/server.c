#define _DEFAULT_SOURCE

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include "qio.h"

#define PROJECT_ID 420

#define EXIT_WITH_ERROR(code, msg)      \
    {                                   \
        fprintf(stderr, "%s\n", (msg)); \
        exit((code));                   \
    }

typedef struct {
    int pid;
    int qid;
} ClientInfo_t;

int shouldClose = 0;
ClientInfo_t clientsList[MAX_CLIENTS] = {0};
int clients = 0;

int insertClient(ClientInfo_t clientInfo) {
    if (clients >= MAX_CLIENTS) {
        return -1;
    }

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clientsList[i].pid == clientInfo.pid) {
            return -2;
        }
    }

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clientsList[i].pid == 0) {
            clientsList[i] = clientInfo;
            ++clients;
        }
    }

    return -3;
}

int removeClient(int pid) {
    if (clients < 0) {
        return 0;
    }

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clientsList[i].pid == pid) {
            clientsList[i].pid = 0;
            clientsList[i].qid = 0;
            --clients;
            return 0;
        }
    }

    return -1;
}

int getClientQidByPid(int pid) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clientsList[i].pid == pid) {
            return clientsList[i].qid;
        }
    }

    return -1;
}

void loginClient(QueueMessage_t* msg) {
    assert(msg != NULL);

    ClientInfo_t client;
    client.pid = msg->pid;

    key_t qkey;
    if (sscanf((char*)msg->data, "%d", &qkey) < 0) {
        EXIT_WITH_ERROR(1, "Manformed message");
    }

    int qid = msgget(qkey, 0);
    if (qid == -1) {
        EXIT_WITH_ERROR(1, "Failed to open client queue");
    }

    if (clients >= MAX_CLIENTS) {
        // Refuse connection
        systemv_queue_send_message(qid, CONNECTION_REFUSED, getpid(), 0, NULL);
    } else {
        client.qid = qid;
        insertClient(client);

        systemv_queue_send_message(qid, CONNECTION_ACCEPTED, getpid(), 0, NULL);
    }
}

void logoutClient(QueueMessage_t* msg) {
    assert(msg != NULL);

    int pid = msg->pid;

    removeClient(pid);
}

void mirrorClient(QueueMessage_t* msg) {
    assert(msg != NULL);

    int qid = getClientQidByPid(msg->pid);

    if (qid == -1) {
        EXIT_WITH_ERROR(1, "Malformed message");
    }

    if (systemv_queue_send_message(qid, MIRROR, getpid(), msg->size,
                                   msg->data) != 0) {
        EXIT_WITH_ERROR(1, systemv_get_error_message());
    }
}

#define LINE_LEN_MAX 2048

void calcClient(QueueMessage_t* msg) {
    assert(msg != NULL);

    int qid = getClientQidByPid(msg->pid);

    if (qid == -1 || msg->size == 0) {
        EXIT_WITH_ERROR(1, "Malformed message");
    }

    char dateBuf[LINE_LEN_MAX];
    char command[LINE_LEN_MAX + 13];

    sprintf(command, "echo '%s' | bc", msg->data);

    FILE* date = popen(command, "r");
    fgets(dateBuf, LINE_LEN_MAX, date);
    pclose(date);

    if (systemv_queue_send_message(qid, TIME, getpid(), strlen(dateBuf) + 1,
                                   dateBuf) != 0) {
        EXIT_WITH_ERROR(1, systemv_get_error_message());
    }
}

void timeClient(QueueMessage_t* msg) {
    assert(msg != NULL);

    int qid = getClientQidByPid(msg->pid);

    if (qid == -1) {
        EXIT_WITH_ERROR(1, "Malformed message");
    }

    char dateBuf[LINE_LEN_MAX];
    FILE* date = popen("/bin/date", "r");
    fgets(dateBuf, LINE_LEN_MAX, date);
    pclose(date);

    if (systemv_queue_send_message(qid, TIME, getpid(), strlen(dateBuf) + 1,
                                   dateBuf) != 0) {
        EXIT_WITH_ERROR(1, systemv_get_error_message());
    }
}

void executeMessage(QueueMessage_t* msg) {
    if (msg == NULL) return;

    switch (msg->type) {
        case LOGIN:
            loginClient(msg);
            break;
        case LOGOUT:
            logoutClient(msg);
            break;
        case MIRROR:
            mirrorClient(msg);
            break;
        case CALC:
            calcClient(msg);
            break;
        case TIME:
            timeClient(msg);
            break;
        case END:
            shouldClose = 1;
            break;
        default:
            break;
    }
}

void mainServerLoop(int queueId) {
    QueueMessage_t message;

    struct msqid_ds state;

    while (shouldClose == 0) {  // TODO: handle all requests before leaving!
        if (msgctl(queueId, IPC_STAT, &state) == -1) {
            EXIT_WITH_ERROR(1, "Failed to get current state of public queue");
        }

        // if(state.msg_qnum == 0) break; /* No messages to process. Quitting */

        if (systemv_queue_receive_message(queueId, &message)) {
            EXIT_WITH_ERROR(1, systemv_get_error_message());
        }

        executeMessage(&message);
    }
}

int queueToRemove = -1;

void remove_queue_at_exit(void) { systemv_queue_close(queueToRemove); }

int main(void) {
    char* HOME_VAR = getenv("HOME");

    if (HOME_VAR == NULL) {
        EXIT_WITH_ERROR(1, "Failed to read $HOME valriable")
    }

    key_t queueKey = ftok(getenv("HOME"), PROJECT_ID);

    if (queueKey == -1) {
        EXIT_WITH_ERROR(1, "Unable to create queue key")
    }

    int queueId = systemv_queue_create(queueKey, IPC_CREAT | IPC_EXCL | 0666);
    queueToRemove = queueId;

    const char* err = systemv_get_error_message();

    if (err != NULL) {
        EXIT_WITH_ERROR(1, err);
    }

    atexit(remove_queue_at_exit);

    mainServerLoop(queueId);

    return 0;
}