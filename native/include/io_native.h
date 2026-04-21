#ifndef IO_NATIVE_H
#define IO_NATIVE_H

#include <stddef.h>

/**
 * Native I/O performance functions using size_t handles.
 */
extern "C" {
    long long io_sendfile(size_t out_handle, const char* path, long long offset, long long length);
    int io_set_direct_io(size_t handle, int enabled);
    size_t io_open_file(const char* path, int write_mode);
    void io_close_file(size_t handle);
}

#endif // IO_NATIVE_H
