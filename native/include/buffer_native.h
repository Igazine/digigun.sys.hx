#ifndef BUFFER_NATIVE_H
#define BUFFER_NATIVE_H

#include <stddef.h>
#include "digigun_export.h"

extern "C" {

    /**
     * RingBuffer: Simple fixed-size circular buffer.
     */
    DIGIGUN_API long long ring_buffer_create(int size);
    DIGIGUN_API void      ring_buffer_destroy(long long handle);
    DIGIGUN_API int       ring_buffer_write(long long handle, const void* data, int len);
    DIGIGUN_API int       ring_buffer_read(long long handle, void* data, int len);
    DIGIGUN_API int       ring_buffer_peek(long long handle, void* data, int len);
    DIGIGUN_API int       ring_buffer_skip(long long handle, int len);
    DIGIGUN_API int       ring_buffer_available(long long handle);
    DIGIGUN_API int       ring_buffer_free_space(long long handle);
    DIGIGUN_API void      ring_buffer_clear(long long handle);

    /**
     * BipBuffer: Bipartite circular buffer.
     * Guarantees contiguous memory blocks for read/write.
     */
    DIGIGUN_API long long bip_buffer_create(int size);
    DIGIGUN_API void      bip_buffer_destroy(long long handle);
    
    // Low-level contiguous access
    DIGIGUN_API void*     bip_buffer_reserve(long long handle, int len, int* reserved_len);
    DIGIGUN_API void      bip_buffer_commit(long long handle, int len);
    DIGIGUN_API void*     bip_buffer_get_read_ptr(long long handle, int* available_len);
    DIGIGUN_API void      bip_buffer_decommit(long long handle, int len);

    // Standard high-level API
    DIGIGUN_API int       bip_buffer_write(long long handle, const void* data, int len);
    DIGIGUN_API int       bip_buffer_read(long long handle, void* data, int len);
    DIGIGUN_API int       bip_buffer_peek(long long handle, void* data, int len);
    DIGIGUN_API int       bip_buffer_skip(long long handle, int len);
    DIGIGUN_API int       bip_buffer_available(long long handle);
    DIGIGUN_API int       bip_buffer_free_space(long long handle);
    DIGIGUN_API void      bip_buffer_clear(long long handle);

    /**
     * ChunkedBuffer: Linked-list of native chunks.
     */
    DIGIGUN_API long long chunked_buffer_create(int chunk_size);
    DIGIGUN_API void      chunked_buffer_destroy(long long handle);
    DIGIGUN_API int       chunked_buffer_write(long long handle, const void* data, int len);
    DIGIGUN_API int       chunked_buffer_read(long long handle, void* data, int len);
    DIGIGUN_API int       chunked_buffer_peek(long long handle, void* data, int len);
    DIGIGUN_API int       chunked_buffer_available(long long handle);
    DIGIGUN_API void      chunked_buffer_clear(long long handle);

}

#endif // BUFFER_NATIVE_H
