#include "common.h"

void die(int errcode, const char *msg) {
    fprintf(stderr, "TERMINATING: %s\n", msg);
    exit(errcode);
}

int socketfd = 0;
char *name;

void sigintHandler(int signum) {
    (void)signum;

    fprintf(stderr, "Client shutting down.\n");

    exit(0);
}

void send_message(uint8_t message_type);

void cleanup(void) {
    send_message(UNREGISTER);
    if (shutdown(socketfd, SHUT_RDWR) == -1) {
        fprintf(stderr, "Failed to shutdown Socket.");
    }
    if (close(socketfd) == -1) {
        fprintf(stderr, "Failed to close Socket.");
    }
}

void setup(const char *clientName, const char *socketType, char *url) {
    signal(SIGINT, sigintHandler);

    name = (char *)calloc(strlen(clientName) + 1, sizeof(char));
    strcpy(name, clientName);

    if (strcmp(socketType, "web") == 0) {
        strtok(url, ":");
        char *portStr = strtok(NULL, ":");
        if (portStr == NULL) {
            die(2, "Invalid server address.");
        }

        uint16_t ip = inet_addr(url);
        if ((int16_t)ip == -1) {
            die(2, "Invalid IP address.");
        }

        uint16_t portNumber = 0;
        if (sscanf(portStr, "%hu", &portNumber) < 1 || portNumber < 1024) {
            die(2, "Invalid port number.");
        }

        struct sockaddr_in webAddress;
        memset(&webAddress, 0, sizeof(struct sockaddr_in));

        webAddress.sin_family = AF_INET;
        webAddress.sin_addr.s_addr = htonl(ip);
        webAddress.sin_port = htons(portNumber);

        if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            die(3, "Failed to create web socket.");
        }

        if (connect(socketfd, (const struct sockaddr *)&webAddress, sizeof(webAddress)) == -1) {
            die(3, "Failed to connect to web socket.");
        }
    } else if (strcmp(socketType, "unix") == 0) {
        // Get Unix Path
        const char *unixPath = url;
        if (strlen(unixPath) < 1 || strlen(unixPath) > UNIX_PATH_MAX) {
            die(3, "Invalid unix path.");
        }

        // Init Local Socket
        struct sockaddr_un unixAddress;
        unixAddress.sun_family = AF_UNIX;
        snprintf(unixAddress.sun_path, UNIX_PATH_MAX, "%s", unixPath);

        if ((socketfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
            die(3, "Failed to create unix socket.");
        }

        if (connect(socketfd, (const struct sockaddr *)&unixAddress, sizeof(unixAddress)) == -1) {
            die(3, "Failed to connect to unix socket.");
        }

    } else {
        die(1, "Invalid socket type.");
    }
}

void send_message(uint8_t message_type) {
    uint16_t message_size = (uint16_t)(strlen(name) + 1);

    if (write(socketfd, &message_type, 1) != 1) {
        die(3, "Failed to write message type.");
    }
    
    if (write(socketfd, &message_size, 2) != 2) {
        die(3, "Failed to write message size.");
    }

    if (write(socketfd, name, message_size) != message_size) {
        die(3, "Failed to write name message.");
    }
}

void register_on_server(void) {
    send_message(REGISTER);

    uint8_t message_type;
    if (read(socketfd, &message_type, 1) != 1) {
        die(3, "Failed to read response message type.");
    }

    switch (message_type) {
        case ERR_NAME_ALREADY_REGISTERED:
            die(3, "Name already in use.");
            break;
        case ERR_TOO_MANY_CLIENTS:
            die(3, "Too many clients logged to the server.");
            break;
        case SUCCESS:
            printf("[INFO] Logged with SUCCESS\n");
            break;
        default:
            die(3, "Unpredicted REGISTER behavior.");
    }
}

void handle_request(void) {
    operation_t operation;
    result_t result;
    char buffer[256];

    if (read(socketfd, &operation, sizeof(operation_t)) != sizeof(operation_t)){
        die(3, "Failed to read request message.");
    }

    result.op_num = operation.op_num;
    result.value = 0;

    sprintf(buffer, "echo 'scale=6; %lf %c %lf' | bc", operation.arg1, operation.op, operation.arg2);
    FILE *calc = popen(buffer, "r");
    size_t n = fread(buffer, 1, 256, calc);
    pclose(calc);
    buffer[n - 1] = '\0';
    sscanf(buffer, "%lf", &result.value);

    send_message(RESULT);
    if (write(socketfd, &result, sizeof(result_t)) != sizeof(result_t)) {
        die(3, "Failed to write result message.");
    }
}

void run(void) {
    uint8_t message_type;

    register_on_server();

    while (1) {
        if (read(socketfd, &message_type, 1) != 1) {
            die(3, "Failed to read message type.");
        }
        switch (message_type) {
            case REQUEST:
                handle_request();
                break;
            case PING:
                send_message(PONG);
                break;
            default:
                fprintf(stderr, "[ERROR] Unknown message type.\n");
                break;
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 4) {
        die(1, "Not enough arguments.");
    }

    if (atexit(cleanup) == -1) {
        die(1, "Failed to register atexit function.");
    }

    setup(argv[1], argv[2], argv[3]);

    run();

    return 0;
}
