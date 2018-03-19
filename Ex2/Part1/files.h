#ifndef FILES_H_
#define FILES_H_

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
        FILE* fp;
    } handle;
    SysOrLib type;
    FileOpenMode mode;
} File;

File* open_file(const char* path, FileOpenMode mode, SysOrLib sysorlib);

ssize_t read_file(File* file, char* buffer, size_t buffer_size);

void close_file(File* file);

#endif /* FILES_H_ */