#ifndef LOOP_NATIVE_H
#define LOOP_NATIVE_H

#include <stddef.h>
#include "digigun_export.h"

/**
 * Platform-agnostic Event Loop / Completion Port abstraction.
 */
extern "C" {

    /**
     * Creates a new native event loop handle.
     * macOS: kqueue()
     * Linux: epoll_create1()
     * Windows: CreateIoCompletionPort()
     */
    DIGIGUN_API long long loop_create();

    /**
     * Closes the event loop handle.
     */
    DIGIGUN_API void loop_close(long long handle);

    /**
     * Operation types for the loop.
     */
    typedef enum {
        LOOP_OP_READ = 0,
        LOOP_OP_WRITE = 1,
        LOOP_OP_ACCEPT = 2,
        LOOP_OP_CONNECT = 3
    } loop_op_t;

    /**
     * Callback for completed operations.
     */
    typedef void (*LoopCallback)(void* user_data, int result, int bytes_transferred);

    /**
     * Submits an asynchronous operation to the loop.
     * @param handle The loop handle.
     * @param fd The file descriptor / socket handle.
     * @param op The operation type.
     * @param buffer Pointer to the data buffer.
     * @param len Buffer length.
     * @param callback Function to call on completion.
     * @param user_data Opaque pointer passed back to the callback.
     */
    DIGIGUN_API int loop_submit(long long handle, long long fd, int op, void* buffer, int len, LoopCallback callback, void* user_data);

    /**
     * Polls the loop for completed events.
     * @param handle The loop handle.
     * @param timeout_ms Timeout in milliseconds (-1 for infinite).
     * @return Number of events processed.
     */
    DIGIGUN_API int loop_poll(long long handle, int timeout_ms);

}

#endif // LOOP_NATIVE_H
