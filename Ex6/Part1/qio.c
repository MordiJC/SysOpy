#include "qio.h"

static const char *error = NULL;

static const char *receiveHeadError = "Failed to receive message head";
static const char *receiveBodyError = "Failed to receive message body";
static const char *queueIdError = "Invalid queue id";
static const char *sendMessageTooBig = "Message too big";
static const char *sendHeadError = "Failed to send message head";
static const char *sendBodyError = "Failed to send message body";
static const char * queueCreationError = "Failed to create queue";
static const char * queueClosingFailed = "Failed to close queue";

int systemv_queue_receive_message(int queue_id, QueueMessageHead_t *msg_head,
                                  QueueMessageBodyUnit_t *msg_body) {
    if (msgrcv(queue_id, msg_head, QueueMessageHeadSize, 0, 0) < 0) {
        error = receiveHeadError;
        return -1;
    }

    if (msgrcv(queue_id, msg_body,
               msg_head->bodySize * sizeof(QueueMessageBodyUnit_t), 0, 0) < 0) {
        error = receiveBodyError;
        return -2;
    }

    return 0;
}

int systemv_queue_send_message(int queue_id, MessageType type, pid_t pid,
                               size_t data_size, QueueMessageBodyUnit_t *src) {
    if (queue_id < 0) {
        error = queueIdError;
        return -1;
    }

    if (data_size > MSG_MAX_BODY_SIZE) {
        error = sendMessageTooBig;
        return -2;  // message to big
    }

    QueueMessageHead_t head = {.type = type, .pid = pid, .bodySize = data_size};

    if (msgsnd(queue_id, &head, QueueMessageHeadSize, 0) == -1) {
        error = sendHeadError;
        return -3;  // failed to send message headqueueCreationError
    }

    if (msgsnd(queue_id, src, data_size, 0) == -1) {
        error = sendBodyError;
        return -4;  // failed to send message body
    }

    return 0;
}

int systemv_queue_create(key_t key, int flags) {
    int qid = msgget(key, flags);

    if(qid == -1) {
        error = queueCreationError;
    }

    return qid;
}

int systemv_queue_close(int queue_id) {
    if (queue_id > -1) {
        int result = msgctl(queue_id, IPC_RMID, 0);

        if(result == -1) {
            error = queueClosingFailed;
            return -1;
        }
    }

    return 0;
}

const char * getErrorMessage() {
    const char * ret = error;
    error = NULL;
    return ret;
}