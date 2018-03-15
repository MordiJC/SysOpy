#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>

#include "block_array.h"

static const char *charsList =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";

char *randomString(size_t maxSize) {
  if (maxSize < 1)
    return NULL;

  const size_t charsListLength = strlen(charsList);

  char *const newStr = (char *)malloc(maxSize * sizeof(char));
  const size_t newLength = maxSize - (rand() % (maxSize - 1));

  for (size_t i = 0; i < newLength; ++i) {
    newStr[i] = charsList[rand() % charsListLength];
  }

  for (size_t i = newLength; i <= maxSize; ++i) {
    newStr[i] = 0;
  }

  return newStr;
}

int getPositiveNumber(const char *str) {
  int x = 0;
  if (sscanf(str, "%d", &x) == EOF) {
    return -1;
  }
  return x;
}

enum AllocationMethod { DYNAMIC = 0, STATIC = 1 };

enum CommandType { UNKNOWN = 0, CREATE = 1, SEARCH = 2, REMOVE = 3, ADD = 4, REMOVE_AND_ADD = 5};

typedef struct CommandStruct {
  enum CommandType type;
  int firstArgument;
  int secondArgument;
} Command;

enum CommandType getCommandType(const char * cmd) {
  if(strcmp(cmd, "create_table") == 0) {
    return CREATE;
  } else if (strcmp(cmd, "search_element") == 0) {
    return SEARCH;
  } else if (strcmp(cmd, "remove") == 0) {
    return REMOVE;
  } else if(strcmp(cmd, "add") == 0) {
    return ADD;
  } else if(strcmp(cmd, "remove_and_add") == 0) {
    return REMOVE_AND_ADD;
  } else {
    return UNKNOWN;
  }
}

int getCommandArgumentsNumber(const char * cmd) {
  switch(getCommandType(cmd)) {
    case CREATE:
      return 2;
      break;
    case SEARCH:
    case REMOVE:
    case ADD:
    case REMOVE_AND_ADD:
      return 1;
      break;
    case UNKNOWN:
    default:
      return 0;
      break;
  }
}

Command * parseCommands(int argc, int startIndex, char ** argv, int * commandsNumber, char ** errorMsg) {
  // Validate commands:
  int commandsNum = 0;
  int count = 0;
  for(int i = startIndex; i < argc;) {
    count = getCommandArgumentsNumber(argv[i]);
    if(count == 0) {
      *errorMsg = "Invalid commands";
      return NULL;
    } else if(i + count >= argc) {
      *errorMsg = "Not enough parameters for commands.";
       return NULL;
    }
    i += count + 1;
    commandsNum++;
  }

  *commandsNumber = commandsNum;

  if(commandsNum == 0) {
    return NULL;
  }

  Command * commands = (Command*) calloc(commandsNum, sizeof(Command));

  int currentCommand = 0;
  for(int i = startIndex; i < argc;) {
    commands[currentCommand].type = getCommandType(argv[i]);

    commands[currentCommand].firstArgument = getPositiveNumber(argv[i+1]);

    if(commands[currentCommand].type == CREATE) {
      commands[currentCommand].secondArgument = getPositiveNumber(argv[i+2]);
      i += 3;
    } else {
      i += 2;
    }
    currentCommand++;
  }

  return commands;
}

int processArguments(int argc, char **argv, int *elementsNumber,
                     int *elementSize, enum AllocationMethod *allocationMethod,
                     int *sum, Command ** commands, int * commandsNumber, 
                     char **errorMessage) {
  if (argc < 5) {
    *errorMessage = "Not enough arguments.\n";
    return -1;
  }

  int elementsN = getPositiveNumber(argv[1]);
  int elementS = getPositiveNumber(argv[2]);
  int argSum = getPositiveNumber(argv[4]);

  if (elementsN == -1) {
    *errorMessage = "Invalid enements number.\n";
    return -1;
  }

  if (elementS == -1) {
    *errorMessage = "Invalid enement size.\n";
    return -1;
  }

  if (argSum == -1) {
    *errorMessage = "Invalid ascii sum.\n";
    return -1;
  }

  enum AllocationMethod am;

  if (strcmp(argv[3], "dynamic") == 0) {
    am = DYNAMIC;
  } else if (strcmp(argv[3], "static") == 0) {
    am = STATIC;
  } else {
    *errorMessage =
        "Invalid allocation method provided. Choose: dynamic or static.\n";
    return -1;
  }
  *commands = parseCommands(argc, 5, argv, commandsNumber, errorMessage);

  if(commands == NULL) {
    return -1;
  }

  *elementsNumber = elementsN;
  *elementSize = elementS;
  *sum = argSum;
  *allocationMethod = am;

  return 0;
}

double timeDiffInSeconds(clock_t start, clock_t end) {
  return (double)(end - start) / sysconf(_SC_CLK_TCK);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  srand(time(NULL));

  int elementsNumber;
  int elementSize;
  int asciiSum;
  enum AllocationMethod allocationMethod = 0;
  Command * commands;
  int commandsNumber;
  char *error = "";

  if (processArguments(argc, argv, &elementsNumber, &elementSize,
                       &allocationMethod, &asciiSum, &commands, &commandsNumber,
                       &error) == -1) {
    fprintf(stderr, "%s", error);
    return 1;
  }

  clock_t * realTimeClocks = (clock_t*)malloc(5 * sizeof(clock_t));
  struct tms **tmsTimes = (struct tms**)malloc(5 * sizeof(struct tms *));

  for (int i = 0; i < 5; ++i) {
    tmsTimes[i] = (struct tms *)malloc(sizeof(struct tms *));
  }

  // Initial time
  realTimeClocks[0] = times(tmsTimes[0]);

  BlockArray *currentBA = 0;

  if (allocationMethod == DYNAMIC) {
    if(BlockArray_create(&currentBA, elementsNumber) == -1) {
      fprintf(stderr, "Unable to initialize BlockArray\n");
      return 1;
    }
  } else {
    currentBA = STATIC_BLOCK_ARRAY;
  }

  for (int i = 0; i < elementsNumber; ++i) {
    char *str = randomString(elementSize);
    BlockArray_addBlock(currentBA, i, str, strlen(str));
  }

  // Time after initialization
  realTimeClocks[1] = times(tmsTimes[1]);

  for(int i = 0; i < elementsNumber; i ++) {
    BlockArray_findBlock(currentBA, asciiSum);
  }

  // Time after finding block
  realTimeClocks[2] = times(tmsTimes[2]);

  for (int i = 0; i < elementsNumber; ++i) {
    char *str = randomString(elementSize);
    BlockArray_addBlock(currentBA, i, str, strlen(str));
  }

  // Time after replacing half of blocks
  realTimeClocks[3] = times(tmsTimes[3]);

  for (int i = 0; i < elementsNumber / 2; ++i) {
    BlockArray_removeBlock(currentBA, i);
  }

  // Time after removing half of blocks
  realTimeClocks[4] = times(tmsTimes[4]);

  double summaryExecutionTime = 0.0;
  for(int i = 1; i < 5; ++i) {
    summaryExecutionTime += timeDiffInSeconds(realTimeClocks[i-1], realTimeClocks[i]);
  }

  printf("%-14s\t%-11s\t%-11s\t%-11s\n", "", "User", "System", "Real");
  printf("%-14s\t%-10.8fs\t%-10.8fs\t%-10.8fs\n",
        "Init & fill",
        timeDiffInSeconds(tmsTimes[0]->tms_stime, tmsTimes[1]->tms_stime)/elementsNumber,
        timeDiffInSeconds(tmsTimes[0]->tms_utime, tmsTimes[1]->tms_utime)/elementsNumber,
        timeDiffInSeconds(realTimeClocks[0], realTimeClocks[1])/elementsNumber);

  printf("%-14s\t%-10.8fs\t%-10.8fs\t%-10.8fs\n",
        "Find",
        timeDiffInSeconds(tmsTimes[1]->tms_stime, tmsTimes[2]->tms_stime)/elementsNumber,
        timeDiffInSeconds(tmsTimes[1]->tms_utime, tmsTimes[2]->tms_utime)/elementsNumber,
        timeDiffInSeconds(realTimeClocks[1], realTimeClocks[2])/elementsNumber);
  
  printf("%-14s\t%-10.8fs\t%-10.8fs\t%-10.8fs\n",
        "Replace 1/2",
        timeDiffInSeconds(tmsTimes[2]->tms_stime, tmsTimes[3]->tms_stime)/elementsNumber/2.0,
        timeDiffInSeconds(tmsTimes[2]->tms_utime, tmsTimes[3]->tms_utime)/elementsNumber/2.0,
        timeDiffInSeconds(realTimeClocks[2], realTimeClocks[3])/elementsNumber/2.0);
  
  printf("%-14s\t%-10.8fs\t%-10.8fs\t%-10.8fs\n",
        "Remove 1/2",
        timeDiffInSeconds(tmsTimes[3]->tms_stime, tmsTimes[4]->tms_stime)/elementsNumber/2.0,
        timeDiffInSeconds(tmsTimes[3]->tms_utime, tmsTimes[4]->tms_utime)/elementsNumber/2.0,
        timeDiffInSeconds(realTimeClocks[3], realTimeClocks[4])/elementsNumber/2.0);

  printf("Summary time:   %10.8fs\n", summaryExecutionTime);

  return 0;
}