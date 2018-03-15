#include "block_array.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
  blockArray->blocks[index] = NULL;

  blockArray->blocksSizes[index] = 0;

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

  size_t bestSum = asciiSum(blockArray->blocks[0], blockArray->blocksSizes[0]);
  char *bestBlock = blockArray->blocks[0];

  for (size_t i = 0; i < blockArray->size; ++i) {
    const int currentSum =
        asciiSum(blockArray->blocks[i], blockArray->blocksSizes[i]);

    if (bestSum - asciiSumSearched > currentSum - asciiSumSearched) {
      bestSum = currentSum;
      bestBlock = blockArray->blocks[i];
    }
  }

  return bestBlock;
}
