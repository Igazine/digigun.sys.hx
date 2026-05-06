#ifndef SERVICE_NATIVE_H
#define SERVICE_NATIVE_H

#include <stddef.h>
#include "digigun_export.h"

/**
 * Native declarations for System Service integration.
 */
extern "C" {
    /**
     * Reports status to the system service manager.
     */
    DIGIGUN_API int service_notify(const char* status);

    /**
     * Checks if the application is running as a system service.
     */
    DIGIGUN_API int service_is_available();

    /**
     * Runs the application as a system service.
     */
    typedef void (*ServiceCallback)();
    DIGIGUN_API int service_run(const char* name, ServiceCallback on_start, ServiceCallback on_stop);
}

#endif // SERVICE_NATIVE_H
