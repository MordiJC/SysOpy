#ifndef BLOCK_ARRAY_H_
#define BLOCK_ARRAY_H_

#include <stddef.h>

typedef struct BlockArrayStruct {
  size_t size;
  char **blocks;
  size_t *blocksSizes;
} BlockArray;

extern BlockArray staticBlockArray;

#define STATIC_BLOCK_ARRAY &staticBlockArray

#define STATIC_BLOCK_ARRAY_SIZE 1024

int BlockArray_create(BlockArray **blockArray, size_t blocksNumber);

void BlockArray_destroy(BlockArray *blockArray);

int BlockArray_addBlock(BlockArray *blockArray, int index, const char *source,
                        size_t sourceSize);

int BlockArray_removeBlock(BlockArray *blockArray, size_t index);

const char * BlockArray_findBlock(BlockArray *blockArray, size_t asciiSumSearched);

#endif /* BLOCK_ARRAY_H_ */