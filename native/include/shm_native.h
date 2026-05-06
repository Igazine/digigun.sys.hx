#ifndef SHM_NATIVE_H
#define SHM_NATIVE_H

#include <stddef.h>
#include "digigun_export.h"

extern "C" {
    /**
     * Shared Memory management.
     */
    DIGIGUN_API long long shm_open_segment(const char* name, int size, int writable);
    DIGIGUN_API void shm_close_segment(long long handle);
    DIGIGUN_API int shm_read_segment(long long handle, int offset, char* buffer, int length);
    DIGIGUN_API int shm_write_segment(long long handle, int offset, const char* buffer, int length);
    DIGIGUN_API void shm_unlink_segment(const char* name);
}

#endif // SHM_NATIVE_H
