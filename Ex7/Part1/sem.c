#include "sem.h"

#include <stdlib.h>

#include "defines.h"

int open_semaphore_set(key_t keyval, int numsems) {
    int sid;

    if (!numsems) return (-1);

    if ((sid = semget(keyval, numsems, IPC_CREAT | 0660)) == -1) {
        return (-1);
    }

    return (sid);
}

void semaphore_init(int sid, int semnum, int initval) {
    union semun semopts;

    semopts.val = initval;
    semctl(sid, semnum, SETVAL, semopts);
}

int semaphore_create(int semnum, int id) {
    char *home = getenv("HOME");
    key_t key = ftok(home, id);

    int sid = open_semaphore_set(key, semnum);

    return sid;
}

int semaphore_signal(int sid, int semnum) {
    struct sembuf sem_lock = {semnum, -1, IPC_NOWAIT};
    return semop(sid, &sem_lock, 1);
}

int semaphore_wait(int sid, int semnum) {
    struct sembuf sem_unlock = {semnum, 1, IPC_NOWAIT};
    return semop(sid, &sem_unlock, 1);
}

int semaphore_waitNum(int sid, int semnum, int num) {
    struct sembuf sem_unlock = {semnum, num, IPC_NOWAIT};
    return semop(sid, &sem_unlock, 1);
}

int semaphore_waitForZero(int sid, int semnum) {
    struct sembuf sem_lock = {semnum, 0, 0};
    return semop(sid, &sem_lock, 1);
}

int semaphore_getLocks(int sid, int semnum) {
    return semctl(sid, semnum, GETVAL, 0);
}

int semaphore_remove(int sid) { return semctl(sid, 0, IPC_RMID, 0); }