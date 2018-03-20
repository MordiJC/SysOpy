#include "files.h"

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

File* open_file(const char* path, FileOpenMode mode, SysOrLib sysorlib) {
    File* file = (File*)malloc(sizeof(File));

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

        if (file->handle.fd == -1) {
            free(file);
            return NULL;
        }

    } else {  // LIB
        file->type = LIB;

        const char* fileOpenMode = "";

        if(mode & WRITE_F && mode & READ_F) {
            fileOpenMode = "r+b";
        } else if (mode & WRITE_F) {
            fileOpenMode = "wb";
        } else if (mode & READ_F) {
            fileOpenMode = "rb";
        } else if (mode & APPEND_F) {
            fileOpenMode = "a+b";
        }

        file->handle.fp = fopen(path, fileOpenMode);

        if (file->handle.fp == NULL) {
            perror("IO error");
            free(file);
            return NULL;
        }
    }

    return file;
}

int read_file(File* file, char* buffer, size_t buffer_size) {
    int readBytes = 0;

    if (file->type == SYS) {
        readBytes = read(file->handle.fd, buffer, buffer_size);
    } else if (file->type == LIB) {
        readBytes = fread(buffer, 1, buffer_size, file->handle.fp);

        if (feof(file->handle.fp) || ferror(file->handle.fp)) {
            return -1;
        }
    }

    return readBytes;
}

int read_file_from_offset(File* file, char* buffer, size_t buffer_size,
                          size_t offset) {
    int readBytes = 0;

    if (file->type == SYS) {
        readBytes = pread(file->handle.fd, buffer, buffer_size, offset);
    } else if (file->type == LIB) {
        int pos = ftell(file->handle.fp);
        fseek(file->handle.fp, offset, SEEK_SET);
        readBytes = fread(buffer, buffer_size, 1, file->handle.fp);
        fseek(file->handle.fp, pos, SEEK_SET);

        if (feof(file->handle.fp) || ferror(file->handle.fp)) {
            return -1;
        }
    }

    return readBytes;
}

int write_file(File* file, const char* bytes, size_t size, size_t offset) {
    int bytes_written = 0;

    if (file->type == SYS) {
        bytes_written = pwrite(file->handle.fd, bytes, size, offset);
    } else if (file->type == LIB) {
        int pos = ftell(file->handle.fp);
        fseek(file->handle.fp, offset, SEEK_SET);
        bytes_written = fwrite(bytes, sizeof(char), size, file->handle.fp);
        fseek(file->handle.fp, pos, SEEK_SET);

        if (ferror(file->handle.fp)) {
            return -1;
        }
    }

    return bytes_written;
}

int append_file(File* file, const char* bytes, size_t size) {
    int bytes_written = 0;

    if (file->type == SYS) {
        long int pos = lseek(file->handle.fd, 0, SEEK_CUR);
        lseek(file->handle.fd, 0, SEEK_END);
        bytes_written = write(file->handle.fd, bytes, size);
        lseek(file->handle.fd, pos, SEEK_SET);
    } else if (file->type == LIB) {
        fpos_t pos;
        fgetpos(file->handle.fp, &pos);
        fseek(file->handle.fp, 0, SEEK_END);
        bytes_written = fwrite(bytes, sizeof(char), size, file->handle.fp);
        fsetpos(file->handle.fp, &pos);

        if (ferror(file->handle.fp)) {
            return -1;
        }
    }

    return bytes_written;
}

void close_file(File* file) {
    if (file->type == SYS) {
        close(file->handle.fd);
        file->handle.fd = 0;
        free(file);

    } else if (file->type == LIB) {
        fclose(file->handle.fp);
        file->handle.fp = NULL;
        free(file);
    }
}