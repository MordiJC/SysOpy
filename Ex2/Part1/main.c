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

typedef enum { SYS = 0, LIB = 1 } SysOrLib;

typedef struct {
    union {
        int fd;
        FILE * fp;
    } handle;
    SysOrLib type;
} File;

int createPosixFileModeFromStd(const char * mode) {
    int outputMode = 0;

    
}

File * open_file(const char * path, const char * mode) {

}

void sortFile(const char* filename, SysOrLib sysorlib, int records,
              int recordLength) {
    (void)filename;
    (void)sysorlib;
    (void)records;
    (void)recordLength;
}

void copyFile(const char* filenameIn, const char* filenameOut,
              SysOrLib sysorlib, int records, int recordLength) {
    (void)filenameIn;
    (void)filenameOut;
    (void)sysorlib;
    (void)records;
    (void)recordLength;
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
                "sort <file> <rowsN> <rowSize> [sys|lib]\n"
                "copy <fileIn> <fileOut> <rowsN> <rowSize>\n");
        return 1;
    }

    return 0;
}
