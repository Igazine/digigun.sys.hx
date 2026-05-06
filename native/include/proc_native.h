#ifndef PROC_NATIVE_H
#define PROC_NATIVE_H

#include <stddef.h>
#include "digigun_export.h"

extern "C" {
    /**
     * CPU Affinity
     */
    DIGIGUN_API int proc_get_affinity();
    DIGIGUN_API int proc_set_affinity(int mask);

    /**
     * Scheduler Priority
     */
    DIGIGUN_API int proc_get_priority();
    DIGIGUN_API int proc_set_priority(int priority);
}

#endif // PROC_NATIVE_H
