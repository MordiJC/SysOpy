#ifndef SHM_H_
#define SHM_H_

#include "defines.h"

int sharedmem_create(int segmentSize);

int sharedmem_existing(void);

int sharedmem_remove(int shmid);

void * sharedmem_map(int shmid);

int sharedmem_unmap(const void * shmaddr);

#endif