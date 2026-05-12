#ifndef RT_NATIVE_H
#define RT_NATIVE_H

#include <stddef.h>
#include "digigun_export.h"

extern "C" {
    /**
     * Lock/Unlock physical memory pages.
     */
    DIGIGUN_API int rt_mlockall(int current, int future);
    DIGIGUN_API int rt_munlockall();

    /**
     * Sets up a native crash handler to capture stack traces and minidumps.
     * @param report_path Path to the text report file.
     * @return 0 on success, non-zero on failure.
     */
    DIGIGUN_API int rt_setup_crash_handler(const char* report_path);
}

#endif // RT_NATIVE_H
