#ifndef QIO_H_
#define QUI_H_

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>

#define MSG_MAX_BODY_SIZE 2048
#define MAX_CLIENTS 10

typedef enum {
    LOGIN = 0u,
    MIRROR,
    CALC,
    TIME,
    END,
} MessageType;

typedef struct {
    MessageType type;
    pid_t pid;
    int bodySize;
} QueueMessageHead_t;

typedef char QueueMessageBodyUnit_t;
typedef char QueueMessageBodyUnitPtr_t;

static const size_t QueueMessageHeadSize = sizeof(QueueMessageHead_t);

/* FUNCTION DEFINITIONS */

int systemv_queue_receive_message(int queue_id, QueueMessageHead_t *msg_head,
                                  QueueMessageBodyUnit_t *msg_body);

int systemv_queue_send_message(int queue_id, MessageType type, pid_t pid,
                               size_t data_size, QueueMessageBodyUnit_t *src);

int systemv_queue_create(key_t key, int flags);

int systemv_queue_close(int queue_id);

const char * getErrorMessage();

#endif /* QIO_H_ */