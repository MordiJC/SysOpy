#ifndef FILTER_H_
#define FILTER_H_

#include <stddef.h>
#include <stdbool.h>

typedef struct Filter_Struct {
    size_t c;
    float* data;
} Filter_t;

Filter_t* Filter_loadFromFile(const char* path);

void Filter_destroy(Filter_t * ptr);

#endif