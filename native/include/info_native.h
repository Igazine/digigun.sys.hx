#ifndef INFO_NATIVE_H
#define INFO_NATIVE_H

#include <stddef.h>
#include "digigun_export.h"

extern "C" {
    DIGIGUN_API void info_get_memory(double* total, double* free, double* used);
    DIGIGUN_API double info_get_cpu_usage();
    DIGIGUN_API void info_get_disk(const char* path, double* total, double* free, double* avail);
}

#endif // INFO_NATIVE_H
