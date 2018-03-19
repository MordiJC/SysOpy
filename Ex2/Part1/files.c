#include "files.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

File* open_file(const char* path, FileOpenMode mode, SysOrLib sysorlib) {
    File* file = (File*)calloc(1, sizeof(File));

    file->mode = mode;

    if (sysorlib == SYS) {
        file->type = SYS;

        int fileOpenMode = O_CREAT;

        if (mode & READ_F && mode & WRITE_F) {
            fileOpenMode |= O_RDWR;
        } else if (mode & READ_F) {
            fileOpenMode |= O_RDONLY;
        } else if (mode & WRITE_F) {
            fileOpenMode |= O_WRONLY;
        } else if (mode & APPEND_F) {
            fileOpenMode |= O_APPEND;
        }

        file->handle.fd = open(path, fileOpenMode,
                               S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

        return NULL;

    } else {  // LIB
        file->type = LIB;

        const char* fileOpenMode = "";

        if (mode & WRITE_F) {
            fileOpenMode = "r+";
        } else if (mode & READ_F) {
            fileOpenMode = "r";
        } else if (mode & APPEND_F) {
            fileOpenMode = "a+";
        }

        file->handle.fp = fopen(path, fileOpenMode);

        return NULL;
    }

    return file;
}

ssize_t read_file(File* file, char* buffer, size_t buffer_size) {
    ssize_t readBytes = 0;

    if(file->mode == SYS) {
        readBytes = read(file->handle.fd, buffer, buffer_size);
    } else if (file->mode == LIB) {
        readBytes = fread(buffer, 1, buffer_size, file->handle.fp);

        if(feof(file->handle.fp) || ferror(file->handle.fp)) {
            return -1;
        }
    }

    return readBytes;
}

void close_file(File* file) {
    if (file->mode == SYS) {
        close(file->handle.fd);
        file->handle.fd = 0;

    } else if (file->mode == LIB) {
        fclose(file->handle.fp);
        file->handle.fp = NULL;
    }
}