#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>

#include "block_array.h"
#include "args.h"

static const char *charsList =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";

char *randomString(size_t maxSize) {
  if (maxSize < 1)
    return NULL;

  const size_t charsListLength = strlen(charsList);

  char *const newStr = (char *)calloc(maxSize, sizeof(char));
  const size_t newLength = maxSize - (rand() % (maxSize - 1));

  for (size_t i = 0; i < newLength; ++i) {
    newStr[i] = charsList[rand() % charsListLength];
  }

  return newStr;
}

double timeDiffInSeconds(clock_t start, clock_t end) {
  return (double)(end - start) / sysconf(_SC_CLK_TCK);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  srand(time(NULL));

  enum AllocationMethod allocationMethod = 0;
  Command *commands;
  int commandsNumber;
  char *error = "";

  if (processArguments(argc, argv, &allocationMethod, &commands,
                       &commandsNumber, &error) == -1) {
    fprintf(stderr, "%s", error);
    return 1;
  }

  BlockArray *currentBA = 0;
  clock_t rtcStart;
  clock_t rtcEnd;
  struct tms tmsTimeStart;
  struct tms tmsTimeEnd;

  double summaryExecutionTime = 0.0;

  printf("%-14s\t%-11s\t%-11s\t%-11s\n", "", "User", "System", "Real");
  const char *operation = "";

  int lastCreationBlockSize = 0;

  for (int i = 0; i < commandsNumber; ++i) {
    Command currentCommand = commands[i];

    switch (currentCommand.type) {
    case CREATE:
      rtcStart = times(&tmsTimeStart);

      if (allocationMethod == DYNAMIC) {
        if (BlockArray_create(&currentBA, currentCommand.firstArgument) == -1) {
          fprintf(stderr, "Unable to initialize BlockArray\n");
          return 1;
        }
      } else {
        currentBA = STATIC_BLOCK_ARRAY;
      }

      for (size_t j = 0; j < currentBA->size; j++) {
        char *str = randomString(currentCommand.secondArgument);
        BlockArray_addBlock(currentBA, j, str, strlen(str));
        free(str);
      }

      rtcEnd = times(&tmsTimeEnd);

      lastCreationBlockSize = currentCommand.secondArgument;

      operation = "Init & fill";
      break;
    case SEARCH:
      rtcStart = times(&tmsTimeStart);

      BlockArray_findBlock(currentBA, currentCommand.firstArgument);      

      rtcEnd = times(&tmsTimeEnd);

      operation = "Search";
      break;
    case REMOVE_AND_ADD:
    case ADD:
      if (currentCommand.firstArgument < 0 ||
          currentCommand.firstArgument > (int)currentBA->size) {
        fprintf(stderr,
                "Invalid argument for `add' or `remove_and_add` command.\n");
        break;
      }

      rtcStart = times(&tmsTimeStart);

      for (int j = 0; j < currentCommand.firstArgument; j++) {
        char *str = randomString(lastCreationBlockSize);
        BlockArray_addBlock(currentBA, j, str, strlen(str));
        free(str);
      }

      rtcEnd = times(&tmsTimeEnd);

      operation = "Add";
      break;
    case REMOVE:
      if (currentCommand.firstArgument < 0 ||
          currentCommand.firstArgument > (int)currentBA->size) {
        fprintf(stderr, "Invalid argument for `remove' command.\n");
        break;
      }

      rtcStart = times(&tmsTimeStart);

      for (int j = 0; j < currentCommand.firstArgument; ++j) {
        BlockArray_removeBlock(currentBA, j);
      }

      rtcEnd = times(&tmsTimeEnd);

      operation = "Remove";

      break;
    case UNKNOWN:
    default:
      break;
    }

    summaryExecutionTime += timeDiffInSeconds(rtcStart, rtcEnd);

    printf("%-14s\t%-10.8fs\t%-10.8fs\t%-10.8fs\n", operation,
           timeDiffInSeconds(tmsTimeStart.tms_stime, tmsTimeEnd.tms_stime),
           timeDiffInSeconds(tmsTimeStart.tms_utime, tmsTimeEnd.tms_utime),
           timeDiffInSeconds(rtcStart, rtcEnd));
  }

  printf("Summary time:   %10.8fs\n", summaryExecutionTime);

  return 0;
}