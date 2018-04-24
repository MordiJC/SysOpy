#include "qio.h"

#include <errno.h>
#include <string.h>

static const char *error = NULL;

static const char *receiveError = "Failed to receive message";
static const char *queueIdError = "Invalid queue id";
static const char *sendMessageTooBig = "Message too big";
static const char *sendError = "Failed to send message";
static const char *queueCreationError = "Failed to create queue";
static const char *queueClosingFailed = "Failed to close queue";
static const char *queueAlreadyExists = "Queue already exists";

int systemv_queue_receive_message(int queue_id, QueueMessage_t *msg) {
    if (msgrcv(queue_id, msg, QueueMessageSize, 0, 0) < 0) {
        error = receiveError;
        return -1;
    }

    return 0;
}

int systemv_queue_send_message(int queue_id, MessageType type, pid_t pid,
                               size_t data_size, char *src) {
    if (queue_id < 0) {
        error = queueIdError;
        return -1;
    }

    if (data_size > MSG_MAX_BODY_SIZE) {
        error = sendMessageTooBig;
        return -2;  // message to big
    }

    QueueMessage_t msg = {.type = type, .pid = pid, .size = data_size};

    if (data_size > 0) {
        memcpy(msg.data, src, data_size);
    } else {
        msg.data[0] = 0;
    }

    if (msgsnd(queue_id, &msg, QueueMessageSize, 0) == -1) {
        error = sendError;
        return -3;  // failed to send message
    }

    return 0;
}

int systemv_queue_create(key_t key, int flags) {
    int qid = msgget(key, flags);

    if (qid == -1) {
        if (errno == EEXIST) {
            error = queueAlreadyExists;
        } else {
            error = queueCreationError;
        }
    }

    return qid;
}

int systemv_queue_close(int queue_id) {
    if (queue_id > -1) {
        int result = msgctl(queue_id, IPC_RMID, 0);

        if (result == -1) {
            error = queueClosingFailed;
            return -1;
        }
    }

    return 0;
}

const char *systemv_get_error_message(void) {
    const char *ret = error;
    error = NULL;
    return ret;
}