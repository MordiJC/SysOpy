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
static const char *queueUnlinkFailed = "Failed to unlink queue";

mqd_t posix_queue_create(const char *path, int flags, int mode) {
    struct mq_attr attr;
    attr.mq_maxmsg = MQ_MAXMSG;
    attr.mq_msgsize = QueueMessageSize;

    mqd_t qid = mq_open(path, flags, mode, &attr);

    if (qid == -1) {
        if (errno == EEXIST) {
            error = queueAlreadyExists;
        } else {
            error = queueCreationError;
        }
    }

    return qid;
}

mqd_t posix_queue_open(const char * path, int flags) {
    mqd_t qid = mq_open(path, flags);

    if (qid == -1) {
        if (errno == EEXIST) {
            error = queueAlreadyExists;
        } else {
            error = queueCreationError;
        }
    }

    return qid;
}

void posix_queue_close(mqd_t queue_id) {
    if (queue_id > -1) {
        if (mq_close(queue_id) == -1) {
            error = queueClosingFailed;
        }
    }
}

void posix_queue_unlink(const char *path) {
    if (mq_unlink(path) == -1) {
        error = queueUnlinkFailed;
    }
}

int posix_queue_receive_message(mqd_t queue_id, QueueMessage_t *msg) {
    if (mq_receive(queue_id, (char *)msg, QueueMessageSize, NULL) == -1) {
        error = receiveError;
        return -1;
    }

    return 0;
}

int posix_queue_send_message(mqd_t queue_id, MessageType type, pid_t pid,
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

    if (mq_send(queue_id, (char *)&msg, QueueMessageSize, 1) == -1) {
        error = sendError;
        return -3;  // failed to send message
    }

    return 0;
}

const char *posix_get_error_message(void) {
    const char *ret = error;
    error = NULL;
    return ret;
}