#include "queue.h"


#include <string.h>

void queue_init(Queue_t* queue, void* address, int capacity, int element_size) {
    queue->mem = address;
    queue->capacity = capacity;
    queue->element_size = element_size;
    queue->head = 0;
    queue->tail = 0;
    queue->elements = 0;
}

bool queue_isFull(Queue_t* queue) {
    if (queue == NULL) return false;

    if ((queue->head % queue->capacity) ==
        ((queue->tail + 1) % queue->capacity)) {
        return true;
    }

    return false;
}

bool queue_isEmpty(Queue_t* queue) {
    if (queue == NULL) return true;

    if (queue->head == queue->tail) {
        return true;
    }

    return false;
}

bool queue_enqueue(Queue_t* queue, void* element) {
    if (queue == NULL || element == NULL) {
        return false;
    }

    if (queue_isFull(queue)) {
        return false;  // FULL
    }

    void * dest = ((char*)queue->mem + queue->tail * queue->element_size);

    memcpy(dest, element,
           queue->element_size);

    queue->tail = (queue->tail + 1) % queue->capacity;

    queue->elements++;

    return true;
}

bool queue_dequeue(Queue_t* queue, void * dest) {
    if(queue_isEmpty(queue)) {
        return false;
    }

    void* element = ((char*)queue->mem + queue->head * queue->element_size);

    queue->head = (queue->head + 1) % queue->capacity;

    memcpy(dest, element, queue->element_size);

    queue->elements--;
    
    return true;
}