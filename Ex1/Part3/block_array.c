#include "block_array.h"

#include <stdio.h>
#include <stdlib.h>

#if defined(LOAD_LIBRARY_AT_RUNTIME)

#include <dlfcn.h>

void * dynLibHandle = NULL:

void BlockArray_exit() {
  if(dynLibHandle != NULL) {
    dlclose(dynLibHandle);
  }
}

void BlockArray_error(const char * err) {
  if(err) {
    fprintf(stderr, "Unable to load symbol from library:\n%s", err);
    dlclose(dynLibHandle);
    exit(1);
  }
}

void BlockArray_init() {
  if(dynLibHandle != NULL) {
    return;
  }

  dynLibHandle = dlopen("./libblockarray.so", RTLD_LAZY);

  if(dynLibHandle == NULL) {
    fprintf(stderr, "Unable to load libblockarray.so:\n%s\n", dlerror());
    exit(1);
  }

  dlerror(); // reset errors

  const char * dlsym_error;

  staticBlockArray = (BlockArray*) dlsym(dynLibHandle, "staticBlockArray");

  BlockArray_create = (BlockArray_create_t) dlsym(dynLibHandle, "BlockArray_create");

  BlockArray_error(dlerror());

  BlockArray_destroy = (BlockArray_destroy_t) dlsym(dynLibHandle, "BlockArray_destroy");

  BlockArray_error(dlerror());

  BlockArray_addBlock = (BlockArray_addBlock_t) dlsym(dynLibHandle, "BlockArray_addBlock");

  BlockArray_error(dlerror());

  BlockArray_removeBlock = (BlockArray_removeBlock_t) dlsym(dynLibHandle, "BlockArray_removeBlock");

  BlockArray_error(dlerror());
  
  BlockArray_findBlock = (BlockArray_findBlock_t) dlsym(dynLibHandle, "BlockArray_findBlock");

  BlockArray_error(dlerror());

  atexit(BlockArray_exit);
}

#else

#include <math.h>
#include <string.h>

void BlockArray_init(void) {}

char *staticBlockArrayBlocks[STATIC_BLOCK_ARRAY_SIZE] = {0};
size_t staticBlockArrayBlocksSize[STATIC_BLOCK_ARRAY_SIZE] = {0};

BlockArray staticBlockArray = {STATIC_BLOCK_ARRAY_SIZE, staticBlockArrayBlocks,
                               staticBlockArrayBlocksSize};

int BlockArray_create(BlockArray **blockArray, size_t blocksNumber) {
  if (*blockArray == STATIC_BLOCK_ARRAY) {
    return -1;
  }

  if (blocksNumber < 1) {
    return -1;
  }

  BlockArray *result = (BlockArray *)calloc(1, sizeof(BlockArray));

  result->blocks = (char **)calloc(blocksNumber, sizeof(char *));
  result->size = blocksNumber;

  result->blocksSizes = (size_t *)calloc(blocksNumber, sizeof(size_t));

  *blockArray = result;

  return 0;
}

void BlockArray_destroy(BlockArray *blockArray) {
  if (blockArray == STATIC_BLOCK_ARRAY) {
    return;
  }

  if (blockArray == NULL) {
    return;
  }

  if (blockArray->blocks != NULL) {
    for (size_t i = 0; i < blockArray->size; ++i) {
      free(blockArray->blocks[i]);
    }
  }

  free(blockArray->blocksSizes);

  free(blockArray);
}

int BlockArray_addBlock(BlockArray *blockArray, int index, const char *source,
                        size_t sourceSize) {
  if (sourceSize <= 0) {
    return -1;
  }

  if (BlockArray_removeBlock(blockArray, index) == -1) {
    return -1;
  }

  if (blockArray->blocksSizes == NULL) {
    return -1;
  }

  blockArray->blocksSizes[index] = sourceSize; // ?
  blockArray->blocks[index] = (char*)calloc(sourceSize, sizeof(char));

  memcpy(blockArray->blocks[index], source, sourceSize);

  return 0;
}

int BlockArray_removeBlock(BlockArray *blockArray, size_t index) {

  if (blockArray == NULL) {
    return -1;
  }

  if (index > blockArray->size) {
    return -1;
  }

  if (blockArray->blocks == NULL) {
    return -1;
  }

  free(blockArray->blocks[index]);

  return 0;
}

size_t asciiSum(const char *str, size_t size) {
  size_t sum = 0;
  for (size_t i = 0; i < size; ++i) {
    sum += (size_t)((unsigned char)str[i]);
  }

  return sum;
}

const char *BlockArray_findBlock(BlockArray *blockArray, size_t asciiSumSearched) {
  if (blockArray == NULL) {
    return NULL;
  }

  if (blockArray->blocks == NULL) {
    return NULL;
  }

  if (blockArray->blocksSizes == NULL) {
    return NULL;
  }

  int bestSum = asciiSum(blockArray->blocks[0], blockArray->blocksSizes[0]);
  char *bestBlock = blockArray->blocks[0];

  for (size_t i = 0; i < blockArray->size; ++i) {
    const int currentSum =
        asciiSum(blockArray->blocks[i], blockArray->blocksSizes[i]);

    if (abs(bestSum - asciiSumSearched) > abs(currentSum - asciiSumSearched)) {
      bestSum = currentSum;
      bestBlock = blockArray->blocks[i];
    }
  }

  return bestBlock;
}

#endif