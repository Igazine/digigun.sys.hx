#ifndef INFO_NATIVE_H
#define INFO_NATIVE_H

/**
 * Declares system information system calls.
 */
extern "C" {
    void info_get_memory(double* total, double* free, double* used);
    double info_get_cpu_usage();
    void info_get_disk(const char* path, double* total, double* free, double* avail);
}

#endif // INFO_NATIVE_H
