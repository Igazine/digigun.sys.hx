#ifndef INFO_NATIVE_H
#define INFO_NATIVE_H

#include <stddef.h>
#include "digigun_export.h"

extern "C" {
    DIGIGUN_API void info_get_memory(double* total, double* free, double* used);
    DIGIGUN_API double info_get_cpu_usage();
    DIGIGUN_API void info_get_disk(const char* path, double* total, double* free, double* avail);

    /**
     * Volume Metadata
     */
    struct NativeVolumeInfo {
        long long total_space;
        long long free_space;
        char file_system[32]; // e.g., "NTFS", "apfs", "ext4"
        char name[128];       // Volume Label
        char uuid[64];        // Volume Serial / UUID
    };

    DIGIGUN_API int info_get_volume_info(const char* path, void* out);
}

#endif // INFO_NATIVE_H
