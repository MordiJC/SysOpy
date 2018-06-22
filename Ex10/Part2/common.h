#ifndef COMMON_H_
#define COMMON_H_

#define _DEFAULT_SOURCE

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <signal.h>

// redefined bcs VSCode didn't have access to it in sys/un.h :/
#define UNIX_PATH_MAX 108

#define CLIENTS_MAX 64

typedef struct {
    uint32_t op_num;
    double value;
} result_t;

typedef struct {
    uint32_t op_num;
    char op;
    double arg1;
    double arg2;
} operation_t;

typedef enum  {
    LOCAL,
    WEB
} connection_t;

typedef struct {
    char *name;
    uint8_t inactiveTicks;
    struct sockaddr * sockaddr;
    socklen_t socklen;
    connection_t connection_type;
} client_t;

typedef enum {
    NONE = 0u,
    REGISTER = 1u,
    UNREGISTER,
    SUCCESS,
    ERR_TOO_MANY_CLIENTS,
    ERR_NAME_ALREADY_REGISTERED,
    REQUEST,
    RESULT,
    PING,
    PONG,
} message_type_t;

typedef struct {
    message_type_t message_type;
    char name[64];
    connection_t connect_type;
    int op_num;
    double value;
} message_t;

#endif