#ifndef FILES_H_
#define FILES_H_

#define _XOPEN_SOURCE 500

#include <stdio.h>

typedef enum { SYS = 0, LIB = 1 } SysOrLib;

typedef enum {
    READ_F = 1u,
    WRITE_F = (1u << 1),
    APPEND_F = (1u << 2)
} FileOpenMode;

typedef struct {
    union {
        int fd;
        FILE *fp;
    } handle;
    SysOrLib type;
} File;

File *open_file(const char *path, FileOpenMode mode, SysOrLib sysorlib);

int read_file(File *file, char *buffer, size_t buffer_size);

int read_file_from_offset(File *file, char *buffer, size_t buffer_size,
                          size_t offset);

int write_file(File *file, const char *bytes, size_t size, size_t offset);

int append_file(File *file, const char *bytes, size_t size);

void close_file(File *file);

#endif /* FILES_H_ */