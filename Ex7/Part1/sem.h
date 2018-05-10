#ifndef SEM_H_
#define SEM_H_

#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>

#ifdef _SEM_SEMUN_UNDEFINED
union semun {
    int val;                    // <= value for SETVAL
    struct semid_ds *buf;       // <= buffer for IPC_STAT & IPC_SET
    unsigned short int *array;  // <= array for GETALL & SETALL
    struct seminfo *__buf;      // <= buffer for IPC_INFO
};
#endif

void semaphore_init(int sid, int semnum, int initval);

int semaphore_create(int semnum);

int semaphore_signal(int sid, int semnum);

int semaphore_wait(int sid, int semnum);

int semaphore_waitNum(int sid, int semnum, int num);

int semaphore_waitForZero(int sid, int semnum);

int semaphore_getLocks(int sid, int semnum);

int semaphore_remove(int sid);

#endif