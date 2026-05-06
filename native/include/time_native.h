#ifndef TIME_NATIVE_H
#define TIME_NATIVE_H

#include <stddef.h>
#include "digigun_export.h"

extern "C" {
    /**
     * High-resolution monotonic timer.
     */
    DIGIGUN_API double time_stamp();
    DIGIGUN_API long long time_nano_stamp();

    /**
     * Precision sleep.
     * @param nanos Nanoseconds to sleep.
     */
    DIGIGUN_API void time_sleep_nanos(long long nanos);
}

#endif // TIME_NATIVE_H
