#include "files.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define printErrorAndExit(msg)    \
    fprintf(stderr, "%s\n", msg); \
    exit(1)

void readPositiveNumberOrPrintMessageAndExit(const char* input, int* value,
                                             const char* message) {
    if (sscanf(input, "%d", value) <= 0) {
        printErrorAndExit(message);
    }

    if (*value <= 0) {
        printErrorAndExit("Number must be positive");
    }
}

static const char* charsList =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";

char* randomString(size_t maxSize) {
    if (maxSize < 1) return NULL;

    const size_t charsListLength = strlen(charsList);

    char* const newStr = (char*)malloc((maxSize + 1) * sizeof(char));

    for (size_t i = 0; i <= maxSize; ++i) {
        newStr[i] = charsList[rand() % charsListLength];
    }

    newStr[maxSize] = 0;

    return newStr;
}

void generateFile(const char* fileName, int records, int recordLength) {
    FILE* file = fopen(fileName, "wb");

    if (file == NULL) {
        printErrorAndExit("Unable to open file");
    }

    for (int i = 0; i < records; ++i) {
        char* str = randomString(recordLength);

        fwrite(str, sizeof(char), strlen(str), file);

        free(str);
    }

    fclose(file);
}

void sortFile(const char* filename, SysOrLib sysorlib, int records,
              int recordLength) {
    File* file = open_file(filename, READ_F | WRITE_F, sysorlib);

    if (file == NULL) {
        printErrorAndExit("UNABLE TO OPEN FILE");
    }

    char* buffer = malloc(recordLength * sizeof(char));
    char* compareBuffer = malloc(recordLength * sizeof(char));  // A[j]

    for (int i = 1; i < records; ++i) {
        int j = i - 1;

        read_file_from_offset(file, buffer, recordLength, i * recordLength);

        while (j >= 0) {
            read_file_from_offset(file, compareBuffer, recordLength,
                                  j * recordLength);

            if ((unsigned char)compareBuffer[0] <= (unsigned char)buffer[0]) {
                break;
            }

            write_file(file, compareBuffer, recordLength,
                       (j + 1) * recordLength);

            j--;
        }

        write_file(file, buffer, recordLength, (j + 1) * recordLength);
    }

    free(buffer);
    free(compareBuffer);

    close_file(file);
}

void copyFile(const char* filenameIn, const char* filenameOut,
              SysOrLib sysorlib, int records, int recordLength) {
    char* buffer = malloc(recordLength * sizeof(char));

    File* inputFile = open_file(filenameIn, READ_F, sysorlib);
    File* outputFile = open_file(filenameOut, WRITE_F, sysorlib);

    for (int i = 0; i < records; ++i) {
        read_file(inputFile, buffer, recordLength);

        append_file(outputFile, buffer, recordLength);
    }

    free(buffer);

    close_file(inputFile);
    close_file(outputFile);
}

void generateCommand(int argc, char** argv) {
    if (argc < 5) {
        printErrorAndExit("Not enough arguments");
    }

    const char* filename = argv[2];
    int recordAmount = 0;
    int recordLength = 0;

    readPositiveNumberOrPrintMessageAndExit(argv[3], &recordAmount,
                                            "Invalid argument: recordAmount");
    readPositiveNumberOrPrintMessageAndExit(argv[4], &recordLength,
                                            "Invalid argument: recordLength");

    generateFile(filename, recordAmount, recordLength);
}

void sortCommand(int argc, char** argv) {
    if (argc < 6) {
        printErrorAndExit("Not enough arguments");
    }

    const char* filename = argv[2];
    const char* mode = argv[5];
    int recordAmount = 0;
    int recordLength = 0;
    SysOrLib sysorlib = SYS;

    readPositiveNumberOrPrintMessageAndExit(argv[3], &recordAmount,
                                            "Invalid argument: recordAmount");
    readPositiveNumberOrPrintMessageAndExit(argv[4], &recordLength,
                                            "Invalid argument: recordLength");

    if (strcmp(mode, "sys") == 0) {
        sysorlib = SYS;
    } else if (strcmp(mode, "lib") == 0) {
        sysorlib = LIB;
    } else {
        printErrorAndExit("Invalid implementation selected");
    }

    sortFile(filename, sysorlib, recordAmount, recordLength);
}

void copyCommand(int argc, char** argv) {
    if (argc < 7) {
        printErrorAndExit("Not enough arguments");
    }

    const char* firstFileName = argv[2];
    const char* secondFileName = argv[3];
    const char* mode = argv[6];
    int recordAmount = 0;
    int recordLength = 0;
    SysOrLib sysorlib = SYS;

    readPositiveNumberOrPrintMessageAndExit(argv[4], &recordAmount,
                                            "Invalid argument: recordAmount");
    readPositiveNumberOrPrintMessageAndExit(argv[5], &recordLength,
                                            "Invalid argument: recordLength");

    if (strcmp(mode, "sys") == 0) {
        sysorlib = SYS;
    } else if (strcmp(mode, "lib") == 0) {
        sysorlib = LIB;
    } else {
        printErrorAndExit("Invalid implementation selected");
    }

    copyFile(firstFileName, secondFileName, sysorlib, recordAmount,
             recordLength);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printErrorAndExit("Not enough arguments");
    }

    if (strcmp(argv[1], "generate") == 0) {
        generateCommand(argc, argv);

    } else if (strcmp(argv[1], "sort") == 0) {
        sortCommand(argc, argv);

    } else if (strcmp(argv[1], "copy") == 0) {
        copyCommand(argc, argv);
    } else {
        fprintf(stderr,
                "Invalid arguments.\nAvailable commands:\n"
                "generate <file> <rowsN> <rowSize>\n"
                "sort <file> <rowsN> <rowSize> <sys|lib>\n"
                "copy <fileIn> <fileOut> <rowsN> <rowSize> <sys|lib>\n");
        return 1;
    }

    return 0;
}
