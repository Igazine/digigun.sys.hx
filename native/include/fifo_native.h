#ifndef FIFO_NATIVE_H
#define FIFO_NATIVE_H

#include <stddef.h>
#include "digigun_export.h"

extern "C" {
    /**
     * File Descriptor management
     */
    DIGIGUN_API int fd_set_blocking(long long fd, int blocking);
    DIGIGUN_API int fd_read(long long fd, char* buffer, int length);
    DIGIGUN_API int fd_write(long long fd, const char* buffer, int length);
    DIGIGUN_API void fd_close(long long fd);
    DIGIGUN_API int fd_get_numeric(long long fd);
    DIGIGUN_API int fd_poll(long long* fds, int* events, int* revents, int count, int timeout_ms);

    /**
     * Named Pipes (FIFO)
     */
    DIGIGUN_API int fifo_create(const char* path, int mode);
    DIGIGUN_API long long fifo_open(const char* path, int write_mode);

    /**
     * Unix Domain Sockets
     */
    DIGIGUN_API long long socket_create();
    DIGIGUN_API int socket_bind(long long fd, const char* path);
    DIGIGUN_API int socket_listen(long long fd, int backlog);
    DIGIGUN_API long long socket_accept(long long fd);
    DIGIGUN_API int socket_connect(long long fd, const char* path);
}

#endif // FIFO_NATIVE_H
