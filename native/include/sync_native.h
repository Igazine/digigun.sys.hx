#ifndef SYNC_NATIVE_H
#define SYNC_NATIVE_H

#include <stddef.h>

/**
 * Native inter-process synchronization using size_t handles.
 */
extern "C" {
    size_t sync_sem_open(const char* name, int initial_value);
    int sync_sem_wait(size_t handle);
    int sync_sem_post(size_t handle);
    int sync_sem_trywait(size_t handle);
    void sync_sem_close(size_t handle);
    void sync_sem_unlink(const char* name);
}

#endif // SYNC_NATIVE_H
