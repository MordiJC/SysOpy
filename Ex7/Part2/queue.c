#include "queue.h"


#include <string.h>

// void queue_init(Queue_t* queue, void* address, QueueInfo_t qi) {
//     queue->mem = address;
//     queue->info = qi;
//     // queue->capacity = capacity;
//     // queue->element_size = element_size;
//     // queue->head = 0;
//     // queue->tail = 0;
//     // queue->elements = 0;
// }

bool queue_isFull(Queue_t* queue) {
    if (queue == NULL) return false;

    if ((queue->info->head % queue->info->capacity) ==
        ((queue->info->tail + 1) % queue->info->capacity)) {
        return true;
    }

    return false;
}

bool queue_isEmpty(Queue_t* queue) {
    if (queue == NULL) return true;

    if (queue->info->head == queue->info->tail) {
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

    void * dest = NULL;

    if(queue_isEmpty(queue) && queue->info->chairOccupied == false) {
        dest = queue->chair;
        queue->info->chairOccupied = true;
    } else {
        dest = ((char*)queue->mem + queue->info->tail * queue->info->element_size);
        queue->info->tail = (queue->info->tail + 1) % queue->info->capacity;
    }

    memcpy(dest, element,
           queue->info->element_size);

    queue->info->elements++;

    return true;
}

bool queue_dequeue(Queue_t* queue, void * dest) {
    if(queue->info->chairOccupied == true) {
        memcpy(dest, queue->chair, queue->info->element_size);
        queue->info->elements--;

        if(queue_isEmpty(queue)) {
            queue->info->chairOccupied = false;
        } else {
            void* element = ((char*)queue->mem + queue->info->head * queue->info->element_size);

            queue->info->head = (queue->info->head + 1) % queue->info->capacity;

            memcpy(queue->chair, element, queue->info->element_size);
        }
    } else {
        return false;
    }
    
    return true;
}