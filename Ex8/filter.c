#include "filter.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

Filter_t* Filter_loadFromFile(const char* path) {
    assert(path != NULL);

    int c = 0;
    FILE* inFile = fopen(path, "r");

    if (inFile == NULL) {
        return NULL;
    }

    if (fscanf(inFile, "%d", &c) < 0) {
        fclose(inFile);
        return NULL;
    }

    Filter_t * filter = (Filter_t*) calloc(1, sizeof(Filter_t));
    filter->c = c;
    filter->data = (float*) calloc(c*c, sizeof(float));

    const size_t c2 = c*c;

    for(size_t i = 0; i < c2; ++i) {
        if (fscanf(inFile, "%f", &(filter->data[i])) < 1) {
            fclose(inFile);
            free(filter->data);
            free(filter);
            return NULL;
        }
    }

    return filter;
}

void Filter_destroy(Filter_t * ptr) {
    if(ptr == NULL) {
        return;
    }

    if(ptr->data != NULL) {
        free(ptr->data);
    }

    free(ptr);
}