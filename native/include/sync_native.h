#ifndef SYNC_NATIVE_H
#define SYNC_NATIVE_H

#include <stddef.h>
#include "digigun_export.h"

extern "C" {
    /**
     * Named Semaphore management.
     */
    DIGIGUN_API long long sync_sem_open(const char* name, int initial_value);
    DIGIGUN_API int sync_sem_wait(long long handle);
    DIGIGUN_API int sync_sem_trywait(long long handle);
    DIGIGUN_API int sync_sem_post(long long handle);
    DIGIGUN_API void sync_sem_close(long long handle);
    DIGIGUN_API void sync_sem_unlink(const char* name);

    /**
     * Futex / WaitOnAddress (Ultra-low-level synchronization)
     */
    DIGIGUN_API int sync_futex_wait(int* addr, int expected_value);
    DIGIGUN_API int sync_futex_wake(int* addr);
    DIGIGUN_API int sync_futex_wake_all(int* addr);
}

#endif // SYNC_NATIVE_H
