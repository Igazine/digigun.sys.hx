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
}

#endif // RT_NATIVE_H
