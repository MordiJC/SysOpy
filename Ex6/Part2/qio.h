#ifndef QIO_H_
#define QUI_H_

#include <mqueue.h>
#include <sys/types.h>
#include <unistd.h>

#define MSG_MAX_BODY_SIZE 2048
#define MAX_CLIENTS 10

#define PROJECT_ID 420

#define LINE_LEN_MAX 2048

#define MQ_MAXMSG 8

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

static const size_t QueueMessageSize = sizeof(QueueMessage_t);

static const char serverPath[] = "/server";

/* FUNCTION DEFINITIONS */

mqd_t posix_queue_create(const char * path, int flags, int mode);

mqd_t posix_queue_open(const char * path, int flags);

void posix_queue_close(mqd_t queue_id);

void posix_queue_unlink(const char *path);

int posix_queue_receive_message(mqd_t queue_id, QueueMessage_t *msg);

int posix_queue_send_message(mqd_t queue_id, MessageType type, pid_t pid,
                               size_t data_size, char *src);


const char *posix_get_error_message(void);

#endif /* QIO_H_ */