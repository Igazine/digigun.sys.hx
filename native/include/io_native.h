#ifndef IO_NATIVE_H
#define IO_NATIVE_H

#include <stddef.h>
#include "digigun_export.h"

extern "C" {
    /**
     * Optimized zero-copy file transfer.
     */
    DIGIGUN_API long long io_sendfile(long long out_handle, const char* path, long long offset, long long length);

    /**
     * Handle-level Direct I/O control.
     */
    DIGIGUN_API int io_set_direct_io(long long handle, int enable);

    /**
     * File Opening/Closing
     */
    DIGIGUN_API long long io_open_file(const char* path, int write_mode);
    DIGIGUN_API void io_close_file(long long handle);
}

#endif // IO_NATIVE_H
