#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "args.h"
#include "block_array.h"

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

#define getTimes(tv, ru)                                                       \
  gettimeofday(tv, NULL);                                                      \
  getrusage(RUSAGE_SELF, ru)

double timevalToSec(struct timeval *tv) {
  return (double)(tv->tv_sec) + ((double)tv->tv_usec / 1000000.0);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  enum AllocationMethod allocationMethod = 0;
  Command *commands;
  int commandsNumber;
  char *error = "";

  BlockArray *currentBA = 0;
  struct timeval tvStart;
  struct timeval tvEnd;
  struct rusage ruStart;
  struct rusage ruEnd;

  double summaryExecutionTime = 0.0;

  FILE *logFile = NULL;

  const char *operation = "";

  int lastCreationBlockSize = 0;

  srand(time(NULL));

  if (processArguments(argc, argv, &allocationMethod, &commands,
                       &commandsNumber, &error) == -1) {
    fprintf(stderr, "%s\n", error);
    return 1;
  }

  logFile = fopen("raport2.txt", "w");

  if (logFile == NULL) {
    fprintf(stderr, "Unable to open log file: `raport2.txt'\n");
    return 1;
  }

  printf("%-14s\t%-11s\t%-11s\t%-11s\n", "", "User", "System", "Real");
  fprintf(logFile, "%-14s\t%-11s\t%-11s\t%-11s\n", "", "User", "System",
          "Real");

  BlockArray_init();

  for (int i = 0; i < commandsNumber; ++i) {
    Command currentCommand = commands[i];

    memset(&tvStart, 0, sizeof(struct timeval));
    memset(&tvEnd, 0, sizeof(struct timeval));
    memset(&ruStart, 0, sizeof(struct rusage));
    memset(&ruEnd, 0, sizeof(struct rusage));

    switch (currentCommand.type) {
    case CREATE:

      getTimes(&tvStart, &ruStart);

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

      getTimes(&tvEnd, &ruEnd);

      lastCreationBlockSize = currentCommand.secondArgument;

      operation = "Init & fill";
      break;
    case SEARCH:
      getTimes(&tvStart, &ruStart);

      BlockArray_findBlock(currentBA, currentCommand.firstArgument);

      getTimes(&tvEnd, &ruEnd);

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

      getTimes(&tvStart, &ruStart);

      for (int j = 0; j < currentCommand.firstArgument; j++) {
        char *str = randomString(lastCreationBlockSize);
        BlockArray_addBlock(currentBA, j, str, strlen(str));
        free(str);
      }

      getTimes(&tvEnd, &ruEnd);

      operation = "Add";
      break;
    case REMOVE:
      if (currentCommand.firstArgument < 0 ||
          currentCommand.firstArgument > (int)currentBA->size) {
        fprintf(stderr, "Invalid argument for `remove' command.\n");
        break;
      }

      getTimes(&tvStart, &ruStart);

      for (int j = 0; j < currentCommand.firstArgument; ++j) {
        BlockArray_removeBlock(currentBA, j);
      }

      getTimes(&tvEnd, &ruEnd);

      operation = "Remove";

      break;
    case UNKNOWN:
    default:
      break;
    }

    summaryExecutionTime += timevalToSec(&tvEnd) - timevalToSec(&tvStart);

    printf("%-14s\t%-8fs\t%-8fs\t%-8fs\n", operation,
           timevalToSec(&ruEnd.ru_utime) - timevalToSec(&ruStart.ru_utime),
           timevalToSec(&ruEnd.ru_stime) - timevalToSec(&ruStart.ru_stime),
           timevalToSec(&tvEnd) - timevalToSec(&tvStart));

    fprintf(logFile, "%-14s\t%-8fs\t%-8fs\t%-8fs\n", operation,
            timevalToSec(&ruEnd.ru_utime) - timevalToSec(&ruStart.ru_utime),
            timevalToSec(&ruEnd.ru_stime) - timevalToSec(&ruStart.ru_stime),
            timevalToSec(&tvEnd) - timevalToSec(&tvStart));
  }

  printf("Summary time:   %10.8fs\n", summaryExecutionTime);
  fprintf(logFile, "Summary time:   %10.8fs\n", summaryExecutionTime);

  fclose(logFile);

  return 0;
}