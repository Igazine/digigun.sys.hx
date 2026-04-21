#ifndef SHM_NATIVE_H
#define SHM_NATIVE_H

#include <stddef.h>

/**
 * Native shared memory functions using size_t handles.
 */
extern "C" {
    size_t shm_open_segment(const char* name, int size, int writable);
    void shm_close_segment(size_t handle);
    int shm_read_segment(size_t handle, int offset, char* buffer, int length);
    int shm_write_segment(size_t handle, int offset, const char* buffer, int length);
    void shm_unlink_segment(const char* name);
}

#endif // SHM_NATIVE_H
