#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 700
#include <dirent.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

typedef enum { O_LT = 0, O_EQ, O_GT } OrderOperator;

typedef enum { M_ODIR = 0, M_NFTW } Mode;

typedef struct {
    const char* dirPath;
    OrderOperator op;
    struct tm date;
    Mode mode;
} ProgramArguments;

#define printErrorAndExit(msg)    \
    fprintf(stderr, "%s\n", msg); \
    exit(1)

void parseArguments(int argc, char* argv[], ProgramArguments* pa) {
    if (argc < 3) {
        printErrorAndExit(
            "Help:\n<file_path> <<=>> \"%Y-%m-%d %H:%M:%S\" <odir|nftw>\n");
    }

    if (argc < 5) {
        printErrorAndExit("Not enough arguments");
    }

    // First argument: dir path
    pa->dirPath = argv[1];

    // Second argument: order operator
    if (strcmp(argv[2], "<") == 0) {
        pa->op = O_LT;
    } else if (strcmp(argv[2], "=") == 0) {
        pa->op = O_EQ;
    } else if (strcmp(argv[2], ">") == 0) {
        pa->op = O_GT;
    } else {
        printErrorAndExit("Invalid order operator");
    }

    // Third argument: date
    memset(&pa->date, 0, sizeof(struct tm));
    if (strptime(argv[3], "%Y-%m-%d %H:%M:%S", &pa->date) == argv[3]) {
        printErrorAndExit(
            "Invalid date format. Correct one is: %Y-%m-%d %H:%M:%S");
    }

    if (strcmp(argv[4], "odir") == 0) {
        pa->mode = M_ODIR;
    } else if (strcmp(argv[4], "nftw") == 0) {
        pa->mode = M_NFTW;
    } else {
        printErrorAndExit("Invalid mode specified");
    }
}

char const* sperm(__mode_t mode) {
    char* local_buff = (char*)calloc(10, sizeof(char));
    int i = 0;
    // user permissions
    if ((mode & S_IRUSR) == S_IRUSR)
        local_buff[i] = 'r';
    else
        local_buff[i] = '-';
    i++;
    if ((mode & S_IWUSR) == S_IWUSR)
        local_buff[i] = 'w';
    else
        local_buff[i] = '-';
    i++;
    if ((mode & S_IXUSR) == S_IXUSR)
        local_buff[i] = 'x';
    else
        local_buff[i] = '-';
    i++;
    // group permissions
    if ((mode & S_IRGRP) == S_IRGRP)
        local_buff[i] = 'r';
    else
        local_buff[i] = '-';
    i++;
    if ((mode & S_IWGRP) == S_IWGRP)
        local_buff[i] = 'w';
    else
        local_buff[i] = '-';
    i++;
    if ((mode & S_IXGRP) == S_IXGRP)
        local_buff[i] = 'x';
    else
        local_buff[i] = '-';
    i++;
    // other permissions
    if ((mode & S_IROTH) == S_IROTH)
        local_buff[i] = 'r';
    else
        local_buff[i] = '-';
    i++;
    if ((mode & S_IWOTH) == S_IWOTH)
        local_buff[i] = 'w';
    else
        local_buff[i] = '-';
    i++;
    if ((mode & S_IXOTH) == S_IXOTH)
        local_buff[i] = 'x';
    else
        local_buff[i] = '-';
    return local_buff;
}

void printDirContents(struct dirent* dirEntry) {
    DIR* dir = NULL;
    struct dirent* currentDirEntry = NULL;
    int skip;

    if (strcmp(dirEntry->d_name, "..") == 0 ||
        strcmp(dirEntry->d_name, ".") == 0) {
        return;
    }

    dir = opendir(dirEntry->d_name);

    if (dir == NULL) {
        printErrorAndExit("Unable to open directory!!!");
    }

    while ((currentDirEntry = readdir(dir)) != NULL) {
        skip = 0;

        char * fullRelativePath = calloc(strlen(currentDirEntry->d_name) + strlen(dirEntry->d_name) + 2, sizeof(char));

        strcat(fullRelativePath, dirEntry->d_name);
        strcat(fullRelativePath, "/");
        strcat(fullRelativePath, currentDirEntry->d_name);

        struct stat stbuf;

        stat(fullRelativePath, &stbuf);

        char* path = realpath(fullRelativePath, NULL);
        char* file = basename(fullRelativePath);

        if (strcmp(file, "..") == 0 || strcmp(file, ".") == 0) {
            skip = 1;
        } else if ((currentDirEntry->d_type == DT_LNK &&
                    !S_ISREG(stbuf.st_mode)) ||
                   currentDirEntry->d_type != DT_REG) {
            skip = 1;
        }
        
        if (currentDirEntry->d_type == DT_DIR || S_ISDIR(stbuf.st_mode)) {
            if (strcmp(file, "..") == 0 || strcmp(file, ".") == 0) {
                printDirContents(currentDirEntry);
            }
        }

        if (skip == 0) {
            const char* perms = sperm(stbuf.st_mode);
            printf("%10.10s %10ldB %s \n", perms, stbuf.st_size, path);
        }
        free(path);
    }

    closedir(dir);
}

void printFiles(ProgramArguments* programArgs) {
    DIR* dir = NULL;
    struct dirent* currentDirEntry = NULL;
    int skip;

    dir = opendir(programArgs->dirPath);

    if (dir == NULL) {
        printErrorAndExit("Unable to open directory");
    }

    while ((currentDirEntry = readdir(dir)) != NULL) {
        skip = 0;

        struct stat stbuf;
        stat(currentDirEntry->d_name, &stbuf);
        char* path = realpath(currentDirEntry->d_name, NULL);
        char* file = basename(currentDirEntry->d_name);

        if (strcmp(file, "..") == 0 || strcmp(file, ".") == 0) {
            skip = 1;
        } else if ((currentDirEntry->d_type == DT_LNK &&
                    !S_ISREG(stbuf.st_mode)) ||
                   currentDirEntry->d_type != DT_REG) {
            skip = 1;
        }

        if (currentDirEntry->d_type == DT_DIR || S_ISDIR(stbuf.st_mode)) {
            if (strcmp(file, "..") != 0 && strcmp(file, ".") != 0) {
                printDirContents(currentDirEntry);
            }
        }

        if (skip == 0) {
            const char* perms = sperm(stbuf.st_mode);
            printf("%10.10s %10ldB %s \n", perms, stbuf.st_size, path);
        }
        free(path);
    }

    closedir(dir);
}

int main(int argc, char* argv[]) {
    ProgramArguments programArgs;

    parseArguments(argc, argv, &programArgs);

    if (programArgs.mode == M_ODIR) {
        printFiles(&programArgs);
    }

    return 0;
}