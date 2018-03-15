#ifndef ARGS_H_
#define ARGS_H_

enum AllocationMethod { DYNAMIC = 0, STATIC = 1 };

enum CommandType {
  UNKNOWN = 0,
  CREATE = 1,
  SEARCH = 2,
  REMOVE = 3,
  ADD = 4,
  REMOVE_AND_ADD = 5
};

typedef struct CommandStruct {
  enum CommandType type;
  int firstArgument;
  int secondArgument;
} Command;

int getPositiveNumber(const char *str);

enum CommandType getCommandType(const char *cmd);

int getCommandArgumentsNumber(const char *cmd);

Command *parseCommands(int argc, int startIndex, char **argv,
                       int *commandsNumber, char **errorMsg);

int processArguments(int argc, char **argv,
                     enum AllocationMethod *allocationMethod,
                     Command **commands, int *commandsNumber,
                     char **errorMessage);

#endif /* ARGS_H_ */