#include "block_array.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

char *staticBlockArrayBlocks[STATIC_BLOCK_ARRAY_SIZE] = {0};
size_t staticBlockArrayBlocksSize[STATIC_BLOCK_ARRAY_SIZE] = {0};

BlockArray staticBlockArray = {STATIC_BLOCK_ARRAY_SIZE, staticBlockArrayBlocks,
                               staticBlockArrayBlocksSize};

int BlockArray_create(BlockArray **blockArray, size_t blocksNumber) {
  if (*blockArray != STATIC_BLOCK_ARRAY) {
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
}

void BlockArray_destroy(BlockArray *blockArray) {
  if (blockArray == STATIC_BLOCK_ARRAY) {
    return;
  }

  if (blockArray == NULL) {
    return;
  }

  if (blockArray->blocks != NULL) {
    for (int i = 0; i < blockArray->size; ++i) {
      free(blockArray->blocks[i]);
    }
  }

  if (blockArray->blocksSizes != NULL) {
    for (int i = 0; i < blockArray->size; ++i) {
      free(blockArray->blocksSizes[i]);
    }
  }

  free(blockArray);
}

int BlockArray_addBlock(BlockArray *blockArray, int index, const char *source,
                        size_t sourceSize) {
  if (sourceSize < 0) {
    return -1;
  }

  if (BlockArray_removeBlock(blockArray, index) == -1) {
    return -1;
  }

  if (blockArray->blocksSizes == NULL) {
    return -1;
  }

  blockArray->blocksSizes[index] = sourceSize; // ?
  blockArray->blocks[index] = (char)calloc(sourceSize, sizeof(char));

  memcpy(blockArray->blocks[index], source, sourceSize);

  return 0;
}

int BlockArray_removeBlock(BlockArray *blockArray, int index) {

  if (blockArray == NULL) {
    return -1;
  }

  if (index > blockArray->size || index < 0) {
    return -1;
  }

  if (blockArray->blocks == NULL) {
    return -1;
  }

  free(blockArray->blocks[index]);

  return 0;
}

int asciiSum(const char *str, size_t size) {
  int sum = 0;
  for (int i = 0; i < size; ++i) {
    sum += str[i];
  }

  return sum;
}

const char *BlockArray_findBlock(BlockArray *blockArray, int asciiSumSearched) {
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

  for (int i = 0; i < blockArray->size; ++i) {
    const int currentSum =
        asciiSum(blockArray->blocks[i], blockArray->blocksSizes[i]);

    if (abs(bestSum - asciiSumSearched) > abs(currentSum - asciiSumSearched)) {
      bestSum = currentSum;
      bestBlock = blockArray->blocks[i];
    }
  }

  return bestBlock;
}
