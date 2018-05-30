#ifndef APGM_H_
#define APGM_H_

#include <stddef.h>
#include <stdbool.h>

typedef struct ASCIIPGM_Struct {
    size_t width;
    size_t height;
    unsigned char* data;
} ASCIIPGM_t;

ASCIIPGM_t* ASCIIPGM_create(size_t width, size_t height);

ASCIIPGM_t* ASCIIPGM_loadFromFile(const char* path);

bool ASCIIPGM_saveToFile(ASCIIPGM_t * ptr, const char * path);

void ASCIIPGM_destroy(ASCIIPGM_t * ptr);

#endif