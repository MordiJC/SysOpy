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
#include <float.h>
#include <math.h>
#include <ftw.h>

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

char const* strperm(__mode_t mode) {
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

void my_nftw(const char* dirpath,
             void (*fn)(const char* fpath, const struct stat* sb)) {
    DIR* dir = NULL;
    struct dirent* currentDirEntry = NULL;
    struct stat stbuf;
    size_t dirpathLen = strlen(dirpath);

    dir = opendir(dirpath);

    if (dir == NULL) {
        printErrorAndExit("Unable to open directory.");
    }

    while ((currentDirEntry = readdir(dir)) != NULL) {
        char* filePath = (char*)calloc(
            dirpathLen + 1 + strlen(currentDirEntry->d_name) + 1, sizeof(char));

        strcat(filePath, dirpath);
        strcat(filePath, "/");
        strcat(filePath, currentDirEntry->d_name);

        stat(filePath, &stbuf);

        if (strcmp(currentDirEntry->d_name, "..") == 0 ||
            strcmp(currentDirEntry->d_name, ".") == 0) {
            continue;
        } else if (currentDirEntry->d_type == DT_LNK &&
                   !S_ISREG(stbuf.st_mode)) {
            continue;
        } else if (currentDirEntry->d_type == DT_DIR ||
                   S_ISDIR(stbuf.st_mode)) {
            my_nftw(filePath, fn);
        } else if (S_ISREG(stbuf.st_mode)) {
            fn(filePath, &stbuf);
        }

        free(filePath);
    }

    closedir(dir);
}

ProgramArguments programArgs;
time_t filterDate = 0;

void fun(const char* file, const struct stat* sb) {
    double diff = difftime(filterDate, sb->st_mtime);

    if ((programArgs.op == O_LT && diff > 0) ||
        (programArgs.op == O_EQ && fabs(diff) <= DBL_EPSILON) ||
        (programArgs.op == O_GT && diff < 0)) {
        const char* perms = strperm(sb->st_mode);
        char date[24] = {0};
        struct tm* tme = localtime(&sb->st_mtime);
        strftime(date, 24, "%Y-%m-%d %H:%M:%S", tme);
        printf("%10.10s %10ldB %s %s \n", perms, sb->st_size, date, file);
    }
}

static int
display_info(const char *fpath, const struct stat *sb,
             int tflag, struct FTW *ftwbuf)
{
    struct stat sbx;
    stat(fpath, &sbx);

    (void) ftwbuf;
    if(tflag == FTW_F) {
        fun(fpath, sb);
    }
    return 0;           /* To tell nftw() to continue */
}

int main(int argc, char* argv[]) {
    parseArguments(argc, argv, &programArgs);

    filterDate = mktime(&programArgs.date);
    char* path = realpath(programArgs.dirPath, NULL);

    if (programArgs.mode == M_ODIR) {
        my_nftw(path, fun);
    } else if (programArgs.mode == M_NFTW) {
        nftw(path, display_info, 64, FTW_DEPTH | FTW_PHYS);
    }

    return 0;
}