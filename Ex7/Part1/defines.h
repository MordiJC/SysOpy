#ifndef DEFINES_H_
#define DEFINES_H_

#define PROJECT_ID 420

#define LINE_LEN_MAX 2048

enum BarberSemaphores {
    semBarberReady    = 0,
    semAccessSeats    = 1,
    semCustomersReady = 2,
    semHaircut        = 3,
    BarberSemaphoresNum,
};

#define EXIT_WITH_MSG(code, msg)        \
    {                                   \
        fprintf(stderr, "%s\n", (msg)); \
        exit(code);                     \
    }

#endif
