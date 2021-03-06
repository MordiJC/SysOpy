#define _DEFAULT_SOURCE

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include "qio.h"

#define EXIT_WITH_ERROR(code, msg)      \
    {                                   \
        fprintf(stderr, "%s\n", (msg)); \
        exit((code));                   \
    }

typedef struct {
    int pid;
    mqd_t qid;
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
    char clientPath[LINE_LEN_MAX] = {0};

    if (sscanf((char*)msg->data, "%s", clientPath) < 0) {
        EXIT_WITH_ERROR(1, "Manformed message");
    }

    int qid = mq_open(clientPath, O_WRONLY);
    if (qid == -1) {
        EXIT_WITH_ERROR(1, "Failed to open client queue");
    }

    if (clients >= MAX_CLIENTS) {
        // Refuse connection
        posix_queue_send_message(qid, CONNECTION_REFUSED, getpid(), 0, NULL);
    } else {
        client.qid = qid;
        insertClient(client);

        posix_queue_send_message(qid, CONNECTION_ACCEPTED, getpid(), 0, NULL);
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

    if (posix_queue_send_message(qid, MIRROR, getpid(), msg->size,
                                   msg->data) != 0) {
        EXIT_WITH_ERROR(1, posix_get_error_message());
    }
}

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

    if (posix_queue_send_message(qid, TIME, getpid(), strlen(dateBuf) + 1,
                                   dateBuf) != 0) {
        EXIT_WITH_ERROR(1, posix_get_error_message());
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

    if (posix_queue_send_message(qid, TIME, getpid(), strlen(dateBuf) + 1,
                                   dateBuf) != 0) {
        EXIT_WITH_ERROR(1, posix_get_error_message());
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

void mainServerLoop(mqd_t queueId) {
    QueueMessage_t message;

    struct mq_attr state;

    while (1) {  // TODO: handle all requests before leaving!
        if(mq_getattr(queueId, &state) == -1) {
            EXIT_WITH_ERROR(1, "Couldn't read queue parameters!")
        }

        if (state.mq_curmsgs == 0 && shouldClose == 1) {
            break; /* No messages to process. Quitting */
        }

        if (posix_queue_receive_message(queueId, &message)) {
            EXIT_WITH_ERROR(1, posix_get_error_message());
        }

        executeMessage(&message);
    }
}

mqd_t queueToRemove = -1;

void remove_queue_at_exit(void) { 
    for(int i = 0; i < MAX_CLIENTS; ++i) {
        if(clientsList[i].pid != 0) {
            posix_queue_close(clientsList[i].qid);
            kill(clientsList[i].pid, SIGINT);
        }
    }

    if(queueToRemove != -1) {
        posix_queue_close(queueToRemove);
        posix_queue_unlink(serverPath);
    }
}

void sig_int_handler(int signum) {
    (void) signum;
    exit(2);
}

int main(void) {
    mqd_t queueId = posix_queue_create(serverPath, O_RDONLY | O_CREAT | O_EXCL, 0666);
    queueToRemove = queueId;

    const char* err = posix_get_error_message();

    if (err != NULL) {
        EXIT_WITH_ERROR(1, err);
    }

    atexit(remove_queue_at_exit);

    signal(SIGINT, sig_int_handler);

    mainServerLoop(queueId);

    return 0;
}