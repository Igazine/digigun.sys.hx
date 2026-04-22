#ifndef RANDOM_NATIVE_H
#define RANDOM_NATIVE_H

#include <stddef.h>

extern "C" {
    /**
     * Fills the provided buffer with cryptographically secure random bytes.
     * @return 0 on success, -1 on failure.
     */
    int random_get_bytes(unsigned char* buffer, int length);
}

#endif // RANDOM_NATIVE_H
