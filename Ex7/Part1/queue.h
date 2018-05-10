#ifndef QUEUE_H_
#define QUEUE_H_

#include <stdbool.h>
#include <stddef.h>

typedef struct QueueStruct {
    void* mem;
    int capacity;
    int element_size;
    int head;
    int tail;
    int elements;
} Queue_t;

void queue_init(Queue_t* queue, void* address, int capacity, int element_size);

bool queue_isFull(Queue_t* queue);

bool queue_isEmpty(Queue_t* queue);

bool queue_enqueue(Queue_t* queue, void* element);

bool queue_dequeue(Queue_t* queue, void * dest);

#endif
