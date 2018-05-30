#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "apgm.h"
#include "filter.h"

#define DIE(Code, Msg) { fprintf(stderr, "%s\n", (Msg)); exit((Code)); }

int threadsNum = 0;
char inputPath[256] = {0};
char filterPath[256] = {0};
char outputPath[256] = {0};

ASCIIPGM_t * inputImage = NULL;
ASCIIPGM_t * outputImage = NULL;
Filter_t * filter = NULL;

int filterPixel(int x, int y) {
    long double res = 0.0L;
    size_t c = filter->c;

    for(size_t j = 0; j < c; ++j) {
        size_t newY = round(fmax(0, y - ceil(c/2) + j));
        newY = (newY < inputImage->height) ? newY : (inputImage->height - 1);
        for(size_t i = 0; i < c; ++i) {
            size_t newX = round(fmax(0, x - ceil(c/2) + i));
            newX = (newX < inputImage->width) ? newX : (inputImage->width - 1);

            res += (
                *(inputImage->data + inputImage->width * newY + newX)
            ) * (
                *(filter->data + c * j + i)
            );
        }
    }

    return round(res < 0 ? 0 : res);
}

void runMulithreadedFiltering(void) {
    for(size_t y = 0; y < inputImage->height; ++y) {
        for(size_t x = 0; x < inputImage->width; ++x) {
            *(outputImage->data + outputImage->width * y + x) = 
                filterPixel(x, y);
        }
    }
}

int main(int argc, char * argv[]) {

    if(argc < (4+1)) {
        DIE(1, "Invalid arguments.")
    }

    if(sscanf(argv[1], "%d", &threadsNum) < 1) {
        DIE(1, "Invalid threadsNum number value.")
    }

    if(sscanf(argv[2], "%255s", inputPath) < 1) {
        DIE(1, "Invalid input file path.")
    }

    if(sscanf(argv[3], "%255s", filterPath) < 1) {
        DIE(1, "Invalid filter file path.")
    }

    if(sscanf(argv[4], "%255s", outputPath) < 1) {
        DIE(1, "Invalid output file path.")
    }


    inputImage = ASCIIPGM_loadFromFile(inputPath);

    if(inputImage == NULL) {
        DIE(2, "Unable to load input image.");
    }

    filter = Filter_loadFromFile(filterPath);
    
    if(filter == NULL) {
        ASCIIPGM_destroy(inputImage);
        DIE(2, "Unable to load filter.");
    }

    outputImage = ASCIIPGM_create(inputImage->width, inputImage->height);

    runMulithreadedFiltering();

    if(ASCIIPGM_saveToFile(outputImage, outputPath) == false) {
        Filter_destroy(filter);
        ASCIIPGM_destroy(inputImage);
        ASCIIPGM_destroy(outputImage);
        DIE(3, "Failed to save filtered image.");
    }

    Filter_destroy(filter);
    ASCIIPGM_destroy(inputImage);
    ASCIIPGM_destroy(outputImage);

    return 0;
}