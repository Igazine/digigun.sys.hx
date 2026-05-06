#ifndef PROCESS_NATIVE_H
#define PROCESS_NATIVE_H

#include <stddef.h>
#include "digigun_export.h"

/**
 * Native structure for process information.
 */
struct NativeProcessInfo {
    int pid;
    int ppid;
    char name[256];
    double memory_rss;
    double memory_virtual;
    struct NativeProcessInfo* next;
};

extern "C" {
    DIGIGUN_API int process_get_id();
    DIGIGUN_API int process_is_root();
    DIGIGUN_API int process_get_file_limit();
    DIGIGUN_API int process_set_file_limit(int limit);
    DIGIGUN_API struct NativeProcessInfo* process_list_all();
    DIGIGUN_API void process_free_list(struct NativeProcessInfo* list);
    DIGIGUN_API int process_fork();
}

#endif // PROCESS_NATIVE_H
