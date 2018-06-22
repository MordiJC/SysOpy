#include "common.h"

uint16_t port = 0;
static char * unixSocketPath = 0;

int webSocket = 0;
int unixSocket = 0;
int epoll = 0;

pthread_t pingThread = 0;
pthread_t commandThread = 0;

pthread_mutex_t clientsMutex = PTHREAD_MUTEX_INITIALIZER;

client_t clients[CLIENTS_MAX];
size_t clientsNum = 0;

uint32_t op_num = 1;

void cleanup(void);

void die(int errcode, const char *msg) {
    fprintf(stderr, "TERMINATING: %s\n", msg);
    exit(errcode);
}

void sigintHandler(int signum) {
    (void)signum;

    fprintf(stderr, "Server shutting down.\n");

    exit(0);
}

size_t inList(void *const listStart, size_t listElements, size_t elementSize, void *const value,
              __compar_fn_t cmp_fun) {
    char *base_ptr = (char *)listStart;
    if (listElements > 0) {
        for (size_t i = 0; i < listElements; ++i) {
            if ((*cmp_fun)((void *)value, (void *)(base_ptr + i * elementSize)) == 0) {
                return i;
            }
        }
    }
    return -1;
}

int cmp_name(char *name, client_t *client) { return strcmp(name, client->name); }

void remove_client(size_t i) {
    free(clients[i].name);
    free(clients[i].sockaddr);

    --clientsNum;
    for (size_t j = i; j < clientsNum; ++j) {
        clients[j] = clients[j + 1];
    }
}

void unregister_client(message_t msg) {
    pthread_mutex_lock(&clientsMutex);
    int index = inList((void *const)clients, (size_t)clientsNum, sizeof(client_t), (void *const)msg.name,
                       (__compar_fn_t)cmp_name);
    if (index >= 0) {
        remove_client(index);
        printf("[INFO] Client \"%s\" unregistered.\n", msg.name);
    }
    pthread_mutex_unlock(&clientsMutex);
}

void register_client(int socket, message_t msg, struct sockaddr * sockaddr_struct, socklen_t socklen) {
    uint8_t message_type = NONE;
    pthread_mutex_lock(&clientsMutex);

    if (clientsNum == CLIENTS_MAX) {
        message_type = ERR_TOO_MANY_CLIENTS;
        if (sendto(socket, &message_type, sizeof(message_type), 0, sockaddr_struct, socklen) != 1) {
            char errormsg[1024] = {0};
            sprintf(errormsg, "Failed to write ERR_TOO_MANY_CLIENTS message to client \"%s\"\n", msg.name);
            die(3, errormsg);
        }
        free(sockaddr_struct);
    } else {
        int index = inList((void *const)clients, (size_t)clientsNum, sizeof(client_t), (void *const)msg.name,
                           (__compar_fn_t)cmp_name);

        if (index != -1) {
            message_type = ERR_NAME_ALREADY_REGISTERED;
            if (sendto(socket, &message_type, sizeof(message_type), 0, sockaddr_struct, socklen) != sizeof(message_type)) {
                char errormsg[1024] = {0};
                sprintf(errormsg, "Failed to write ERR_NAME_ALREADY_REGISTERED message to client \"%s\"\n", msg.name);
                die(3, errormsg);
            }
            free(sockaddr_struct);
        } else {
            clients[clientsNum].sockaddr = sockaddr_struct;
            clients[clientsNum].socklen = socklen;
            clients[clientsNum].connection_type = msg.connect_type;

            clients[clientsNum].name = malloc(strlen(msg.name) + 1);
            strcpy(clients[clientsNum].name, msg.name);

            clients[clientsNum].inactiveTicks = 0;

            ++clientsNum;

            message_type = SUCCESS;

            if (sendto(socket, &message_type, sizeof(message_type), 0, sockaddr_struct, socklen) != sizeof(message_type)) {
                perror("SOCKERROR");
                char errormsg[1024] = {0};
                sprintf(errormsg, "Failed to write SUCCESS message to client \"%s\"\n", msg.name);
                die(3, errormsg);
            }
        }
    }
    pthread_mutex_unlock(&clientsMutex);
}

void *pingThreadRunner(void *arg) {
    (void) arg;
    uint8_t message_type = PING;
    while (1) {
        pthread_mutex_lock(&clientsMutex);
        for (size_t i = 0; i < clientsNum; ++i) {
            if (clients[i].inactiveTicks > 0) {
                printf(
                    "Client \"%s\" didn\'t respond. Removing from registered "
                    "clients\n",
                    clients[i].name);
                remove_client(i);
                --i;  // Client "poped"
            } else {
                int sock = clients[i].connection_type == LOCAL ? unixSocket : webSocket;
                if(sendto(sock, &message_type, sizeof(message_type), 0, clients[i].sockaddr, clients[i].socklen) != 1) {
                    char msg[1024] = {0};
                    sprintf(msg, "Failed to PING client `%s`.", clients[i].name);
                    die(3, msg);
                }
                clients[i].inactiveTicks++;
            }
        }
        pthread_mutex_unlock(&clientsMutex);
        sleep(5);
    }

    return (void *)0;
}

void *commandThreadRunner(void *arg) {
    (void) arg;
    srand((unsigned int)time(NULL));

    operation_t operation;
    uint8_t message_type = REQUEST;

    bool error = false;
    char buffer[256];

    while (1) {
        printf("Enter command: \n> ");
        fgets(buffer, 256, stdin);

        if (sscanf(buffer, "%lf %c %lf", &operation.arg1, &operation.op, &operation.arg2) != 3) {
            printf("[ERROR] Wrong command format\n");
            continue;
        }
        if (operation.op != '+' && operation.op != '-' && operation.op != '*' && operation.op != '/') {
            printf("[ERROR] Wrong operator (%c)\n", operation.op);
            continue;
        }

        operation.op_num = op_num;
        ++op_num;
        pthread_mutex_lock(&clientsMutex);
        if (clientsNum == 0) {
            printf("[INFO] No Clients connected to server\n");
            continue;
        }

        error = false;
        int i = rand() % clientsNum;

        int sock = clients[i].connection_type == LOCAL ? unixSocket : webSocket;

        if(sendto(sock, &message_type, sizeof(message_type), 0, clients[i].sockaddr, clients[i].socklen) != sizeof(message_type)) {
            error = true;
        }
        if(sendto(sock, &operation, sizeof(operation), 0, clients[i].sockaddr, clients[i].socklen) != sizeof(operation)) {
            error = true;
        }

        if (error == false) {
            printf(
                "[INFO] Command %d: %lf %c %lf has been sent to client "
                "\"%s\"\n",
                operation.op_num, operation.arg1, operation.op, operation.arg2, clients[i].name);
        } else {
            printf("Failed to send request to client \"%s\"\n", clients[i].name);
        }
        pthread_mutex_unlock(&clientsMutex);
    }

    return (void *)0;
}

void handle_message(int socket) {
    struct sockaddr* sockaddr = (struct sockaddr*)malloc(sizeof(struct sockaddr));
    socklen_t socklen = sizeof(struct sockaddr);
    message_t msg;

    if (recvfrom(socket, &msg, sizeof(msg), 0, sockaddr, &socklen) != sizeof(msg)) {
        die(3, "Failed to receive message\n");
    }

    if(msg.message_type == REGISTER) {
        register_client(socket, msg, sockaddr, socklen);
    } else {
        free(sockaddr);
        switch (msg.message_type) {
            case UNREGISTER: {
                unregister_client(msg);
                break;
            }
            case RESULT: {
                printf("Client \"%s\" calculated operation [%d]. Result: %lf\n", msg.name, msg.op_num, msg.value);
                break;
            }
            case PONG: {
                pthread_mutex_lock(&clientsMutex);
                int index = inList((void *const)clients, (size_t)clientsNum, sizeof(client_t), (void *const)msg.name,
                                (__compar_fn_t)cmp_name);
                if (index >= 0) {
                    clients[index].inactiveTicks--;
                }
                pthread_mutex_unlock(&clientsMutex);
                break;
            }
            default:
                printf("Unknown message type\n");
                break;
        }
    }
}

void setup(void) {
    signal(SIGINT, sigintHandler);
    atexit(cleanup);

    if (port < 1024) {
        die(3, "Invalid port number.");
    }

    size_t upl = strlen(unixSocketPath);
    if (upl < 1 || upl > UNIX_PATH_MAX) {
        die(3, "Invalid unix socket path.");
    }

    // ### TCP SOCKET ###
    struct sockaddr_in webSocketAddress;
    memset(&webSocketAddress, 0, sizeof(struct sockaddr_in));
    webSocketAddress.sin_family = AF_INET;
    webSocketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    webSocketAddress.sin_port = htons(port);

    if ((webSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        die(3, "Failed to create web socket.");
    }

    if (bind(webSocket, (const struct sockaddr *)&webSocketAddress, sizeof(webSocketAddress))) {
        die(3, "Failed to bind web socket.");
    }

    // ### UNIX SOCKET ###
    struct sockaddr_un localAddress;
    localAddress.sun_family = AF_UNIX;

    snprintf(localAddress.sun_path, UNIX_PATH_MAX, "%s", unixSocketPath);

    if ((unixSocket = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        die(3, "Failed to create local socket.");
    }

    if (bind(unixSocket, (const struct sockaddr *)&localAddress, sizeof(localAddress))) {
        die(3, "Failed to bind local socket.");
    }

    // ### EPOLL ###
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLPRI;

    if ((epoll = epoll_create1(0)) == -1) {
        die(3, "Failed to create epoll\n");
    }

    event.data.fd = webSocket;  // TODO: wyjebać -'a jak niedziała z nim
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, webSocket, &event) == -1) {
        die(3, "Failed to add Web Socket to epoll.");
    }

    event.data.fd = unixSocket;  // TODO: wyjebać -'a jak niedziała z nim
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, unixSocket, &event) == -1) {
        die(3, "Failed to add local socket to epoll.");
    }

    // ### PING THREAD START ###
    if (pthread_create(&pingThread, NULL, pingThreadRunner, NULL) != 0) {
        die(3, "Failed to create Ping thread.");
    }

    // ### COMMAND THREAD START ###
    if (pthread_create(&commandThread, NULL, commandThreadRunner, NULL) != 0) {
        die(3, "Failed to create Command thread.");
    }
}

void cleanup(void) {
    if(pingThread != 0) {
        pthread_cancel(pingThread);
    }
    if(commandThread != 0) {
        pthread_cancel(commandThread);
    }
    if (close(webSocket) == -1) {
        fprintf(stderr, "Error: Failed to close web socket.\n");
    }
    if (close(unixSocket) == -1) {
        fprintf(stderr, "Error: Failed to close unix socket.\n");
    }
    if (unlink(unixSocketPath) == -1) {
        fprintf(stderr, "Error: Failed to unlink unix path.\n");
    }
    if (close(epoll) == -1) {
        fprintf(stderr, "Error: Failed to close epoll.\n");
    }
    free(unixSocketPath);
}

void run(void) {
    setup();

    struct epoll_event event;
    while (1) {
        if (epoll_wait(epoll, &event, 1, -1) == -1) {
            die(3, "epoll_wait has failed.");
        }

        handle_message(event.data.fd);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        die(1, "Not enough arguments.");
    }

    if (sscanf(argv[1], "%hu", &port) < 1) {
        die(1, "Invalid argument [1].");
    }

    unixSocketPath = (char*) calloc(strlen(argv[2]) + 1, sizeof(char));
    strcpy(unixSocketPath, argv[2]);

    run();

    return 0;
}