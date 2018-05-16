#include "shm.h"

#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <stdlib.h>

int open_segment(key_t keyval, int segsize) {
    int shmid;

    if ((shmid = shmget(keyval, segsize, IPC_CREAT | 0666)) == -1) {
        return (-1);
    }

    return (shmid);
}

int sharedmem_create(int segmentSize) {
    char * path = getenv("HOME");
    key_t key = ftok(path, PROJECT_ID);

    return open_segment(key, segmentSize);
}

int sharedmem_existing(void) {
    return sharedmem_create(0);
}

int sharedmem_remove(int shmid) {
    return (int)shmctl(shmid, IPC_RMID, NULL);
}

void * sharedmem_map(int shmid) {
    return shmat(shmid, 0, 0);
}

int sharedmem_unmap(const void * shmaddr) {
    return shmdt(shmaddr);
}
