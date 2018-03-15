#include "args.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int getPositiveNumber(const char *str) {
  int x = 0;
  if (sscanf(str, "%d", &x) == EOF) {
    return -1;
  }
  return x;
}

enum CommandType getCommandType(const char *cmd) {
  if (strcmp(cmd, "create_table") == 0) {
    return CREATE;
  } else if (strcmp(cmd, "search_element") == 0) {
    return SEARCH;
  } else if (strcmp(cmd, "remove") == 0) {
    return REMOVE;
  } else if (strcmp(cmd, "add") == 0) {
    return ADD;
  } else if (strcmp(cmd, "remove_and_add") == 0) {
    return REMOVE_AND_ADD;
  } else {
    return UNKNOWN;
  }
}

int getCommandArgumentsNumber(const char *cmd) {
  switch (getCommandType(cmd)) {
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

Command *parseCommands(int argc, int startIndex, char **argv,
                       int *commandsNumber, char **errorMsg) {
  // Validate commands:
  int commandsNum = 0;
  int count = 0;
  for (int i = startIndex; i < argc;) {
    count = getCommandArgumentsNumber(argv[i]);
    if (count == 0) {
      *errorMsg = "Invalid commands";
      return NULL;
    } else if (i + count >= argc) {
      *errorMsg = "Not enough parameters for commands.";
      return NULL;
    }
    i += count + 1;
    commandsNum++;
  }

  *commandsNumber = commandsNum;

  if (commandsNum == 0) {
    return NULL;
  }

  Command *commands = (Command *)calloc(commandsNum, sizeof(Command));

  int currentCommand = 0;
  for (int i = startIndex; i < argc;) {
    commands[currentCommand].type = getCommandType(argv[i]);

    commands[currentCommand].firstArgument = getPositiveNumber(argv[i + 1]);

    if (commands[currentCommand].type == CREATE) {
      commands[currentCommand].secondArgument = getPositiveNumber(argv[i + 2]);
      i += 3;
    } else {
      i += 2;
    }
    currentCommand++;
  }

  return commands;
}

int processArguments(int argc, char **argv,
                     enum AllocationMethod *allocationMethod,
                     Command **commands, int *commandsNumber,
                     char **errorMessage) {
  if (argc < 2) {
    *errorMessage = "Not enough arguments.\n";
    return -1;
  }

  enum AllocationMethod am;

  if (strcmp(argv[1], "dynamic") == 0) {
    am = DYNAMIC;
  } else if (strcmp(argv[1], "static") == 0) {
    am = STATIC;
  } else {
    *errorMessage =
        "Invalid allocation method provided. Choose: dynamic or static.\n";
    return -1;
  }
  *commands = parseCommands(argc, 2, argv, commandsNumber, errorMessage);

  if (commands == NULL) {
    return -1;
  }

  *allocationMethod = am;

  return 0;
}