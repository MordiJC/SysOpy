#include "apgm.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ASCIIPGM_t* ASCIIPGM_create(size_t width, size_t height) {
    ASCIIPGM_t* ret = (ASCIIPGM_t*)calloc(1, sizeof(ASCIIPGM_t));

    ret->data = (unsigned char*)calloc(width * height, sizeof(unsigned char));
    ret->height = height;
    ret->width = width;

    return ret;
}

char lineBufer[1024] = {0};

ASCIIPGM_t* ASCIIPGM_loadFromFile(const char* path) {
    assert(path != NULL);

    memset(lineBufer, 0, 1024);

    int w = 0, h = 0;
    FILE* inFile = fopen(path, "r");

    if (inFile == NULL) {
        return NULL;
    }

    do {
        fscanf(inFile, "%1024[^\n]\n", lineBufer);
    } while(lineBufer[0] == '#');

    if (sscanf(lineBufer, "P2") < 0) {
        fprintf(stderr, "wrong!\n");
        fclose(inFile);
        return NULL;
    }

    do {
        fscanf(inFile, "%1024[^\n]\n", lineBufer);
    } while(lineBufer[0] == '#');

    if(sscanf(lineBufer, "%d %d", &w, &h) != 2) {
        fprintf(stderr, "wrong 2!\n%s\n", lineBufer);
        fclose(inFile);
        return NULL;
    }

    do {
        fscanf(inFile, "%1024[^\n]\n", lineBufer);
    } while(lineBufer[0] == '#');
    
    if(sscanf(lineBufer, "255") < 0) {
        fprintf(stderr, "wrong 3!\n");
        fclose(inFile);
        return NULL;
    }

    const size_t dataWidth = w * h;

    unsigned char* data =
        (unsigned char*)calloc(dataWidth, sizeof(unsigned char));

    for (size_t i = 0; i < dataWidth; ++i) {
        if (fscanf(inFile, "%hhu", &(data[i])) < 1) {
            fclose(inFile);
            free(data);
            return NULL;
        }
    }

    fclose(inFile);

    ASCIIPGM_t* ret = (ASCIIPGM_t*)calloc(1, sizeof(ASCIIPGM_t));

    ret->data = data;
    ret->height = h;
    ret->width = w;

    return ret;
}

bool ASCIIPGM_saveToFile(ASCIIPGM_t * ptr, const char * path) {
    assert(path != NULL);
    assert(ptr != NULL);
    assert(ptr->data != NULL);

    const size_t dataWidth = ptr->height * ptr->width;
    FILE * file = fopen(path, "w");

    if(file == NULL) {
        return false;
    }

    fprintf(file, "P2\n%ld %ld\n255\n", ptr->width, ptr->height);

    for(size_t i = 0; i < dataWidth; ++i) {
        fprintf(file, "%3hhu%c", ptr->data[i], (i != 0 && i % ptr->width == 0 ? '\n' : ' '));
    }

    fclose(file);

    return true;
}

void ASCIIPGM_destroy(ASCIIPGM_t * ptr) {
    if(ptr == NULL) {
        return;
    }

    if(ptr->data != NULL) {
        free(ptr->data);
    }

    free(ptr);
}