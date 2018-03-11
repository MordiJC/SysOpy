#ifndef BLOCK_ARRAY_H_
#define BLOCK_ARRAY_H_

#include <stddef.h>
#include "config.h"

typedef struct BlockArrayStruct {
  size_t size;
  char **blocks;
  size_t *blocksSizes;
} BlockArray;

extern LIBRARY_API BlockArray staticBlockArray;

#define STATIC_BLOCK_ARRAY &staticBlockArray

#define STATIC_BLOCK_ARRAY_SIZE 1024

int LIBRARY_API BlockArray_create(BlockArray **blockArray, size_t blocksNumber);

void LIBRARY_API BlockArray_destroy(BlockArray *blockArray);

int LIBRARY_API BlockArray_addBlock(BlockArray *blockArray, int index, const char *source,
                        size_t sourceSize);

int LIBRARY_API BlockArray_removeBlock(BlockArray *blockArray, int index);

const char * LIBRARY_API BlockArray_findBlock(BlockArray *blockArray, int asciiSumSearched);

#endif /* BLOCK_ARRAY_H_ */