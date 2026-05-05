#ifndef SERVICE_NATIVE_H
#define SERVICE_NATIVE_H

/**
 * Native declarations for System Service integration.
 */
extern "C" {
    /**
     * Reports status to the system service manager.
     * On Linux, implements the sd_notify protocol.
     * On Windows, updates the Service Control Manager status.
     * @return 0 on success, -1 on failure.
     */
    int service_notify(const char* status);

    /**
     * Checks if the application is running as a system service.
     * @return 1 if running as a service, 0 otherwise.
     */
    int service_is_available();

    /**
     * Runs the application as a Windows service.
     * On POSIX, this is a no-op.
     * @param name The service name.
     * @param on_start Callback for service start.
     * @param on_stop Callback for service stop.
     * @return 0 on success.
     */
    typedef void (*ServiceCallback)();
    int service_run(const char* name, ServiceCallback on_start, ServiceCallback on_stop);
}

#endif // SERVICE_NATIVE_H
