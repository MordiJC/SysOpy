#ifndef QIO_H_
#define QUI_H_

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>

#define MSG_MAX_BODY_SIZE 2048
#define MAX_CLIENTS 10

typedef enum {
    LOGIN = 1u,
    LOGOUT,
    MIRROR,
    CALC,
    TIME,
    END,
    CONNECTION_ACCEPTED,
    CONNECTION_REFUSED,
} MessageType;

typedef struct {
    long type;
    pid_t pid;
    int size;
    char data[MSG_MAX_BODY_SIZE];
} QueueMessage_t;

static const size_t QueueMessageSize = sizeof(QueueMessage_t) - sizeof(long);

/* FUNCTION DEFINITIONS */

int systemv_queue_receive_message(int queue_id, QueueMessage_t *msg_head);

int systemv_queue_send_message(int queue_id, MessageType type, pid_t pid,
                               size_t data_size, char *src);

int systemv_queue_create(key_t key, int flags);

int systemv_queue_close(int queue_id);

const char *systemv_get_error_message(void);

#endif /* QIO_H_ */