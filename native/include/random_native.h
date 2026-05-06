#ifndef RANDOM_NATIVE_H
#define RANDOM_NATIVE_H

#include <stddef.h>
#include "digigun_export.h"

extern "C" {
    /**
     * Generates cryptographically secure random bytes.
     * @return 0 on success, -1 on failure.
     */
    DIGIGUN_API int random_get_bytes(unsigned char* buffer, size_t size);
}

#endif // RANDOM_NATIVE_H
