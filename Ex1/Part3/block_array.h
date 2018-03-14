#ifndef BLOCK_ARRAY_H_
#define BLOCK_ARRAY_H_

#include <stddef.h>

typedef struct BlockArrayStruct {
  size_t size;
  char **blocks;
  size_t *blocksSizes;
} BlockArray;

void BlockArray_init(void);

#if defined(LOAD_LIBRARY_AT_RUNTIME)

  BlockArray * staticBlockArray;

  #define STATIC_BLOCK_ARRAY staticBlockArray

  #define STATIC_BLOCK_ARRAY_SIZE 1024

  typedef int (*BlockArray_create_t)(BlockArray **blockArray, size_t blocksNumber)

  typedef void (*BlockArray_destroy_t)(BlockArray *blockArray)

  typedef int (*BlockArray_addBlock_t)(BlockArray *blockArray, int index, const char *source,
                          size_t sourceSize)

  typedef int (*BlockArray_removeBlock_t)(BlockArray *blockArray, size_t index)

  typedef const char * (*BlockArray_findBlock_t)(BlockArray *blockArray, size_t asciiSumSearched)


  BlockArray_create_t BlockArray_create = NULL;

  BlockArray_destroy_t BlockArray_destroy = NULL;

  BlockArray_addBlock_t BlockArray_addBlock = NULL;

  BlockArray_removeBlock_t BlockArray_removeBlock= NULL;

  BlockArray_findBlock_t BlockArray_findBlock = NULL;

#else

  extern BlockArray staticBlockArray;

  #define STATIC_BLOCK_ARRAY &staticBlockArray

  #define STATIC_BLOCK_ARRAY_SIZE 1024

  int BlockArray_create(BlockArray **blockArray, size_t blocksNumber);

  void BlockArray_destroy(BlockArray *blockArray);

  int BlockArray_addBlock(BlockArray *blockArray, int index, const char *source,
                          size_t sourceSize);

  int BlockArray_removeBlock(BlockArray *blockArray, size_t index);

  const char * BlockArray_findBlock(BlockArray *blockArray, size_t asciiSumSearched);

#endif

#endif /* BLOCK_ARRAY_H_ */