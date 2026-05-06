#ifndef SIGNAL_NATIVE_H
#define SIGNAL_NATIVE_H

#include <stddef.h>
#include "digigun_export.h"

extern "C" {
    DIGIGUN_API int signal_raise(int signo);
    DIGIGUN_API int signal_get_value(const char* name);
    DIGIGUN_API int signal_trap(int signo, void* hx_callback_ptr);
}

#endif // SIGNAL_NATIVE_H
