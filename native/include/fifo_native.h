#ifndef FIFO_NATIVE_H
#define FIFO_NATIVE_H

#include <stddef.h>

/**
 * Declares functions for FIFO and Socket operations.
 * Uses size_t for handles to ensure correct hxcpp mapping to cpp.SizeT.
 */
extern "C" {
    int fifo_create(const char* path, int mode);
    size_t fifo_open(const char* path, int write_mode);
    
    size_t socket_create();
    int socket_bind(size_t handle, const char* path);
    int socket_listen(size_t handle, int backlog);
    size_t socket_accept(size_t handle);
    int socket_connect(size_t handle, const char* path);
    
    int fd_read(size_t handle, char* buffer, int length);
    int fd_write(size_t handle, const char* buffer, int length);
    int fd_set_blocking(size_t handle, int blocking);
    
    int fd_poll(size_t* handles, int* events, int* revents, int count, int timeout);
    
    void fd_close(size_t handle);
    int fd_get_numeric(size_t handle);
}

#endif // FIFO_NATIVE_H
