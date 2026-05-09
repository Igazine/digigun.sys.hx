#include <hxcpp.h>
#include "service_native.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winerror.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

// Global state for Windows SCM
static SERVICE_STATUS_HANDLE g_status_handle = NULL;
static SERVICE_STATUS        g_status = {0};
static ServiceCallback       g_on_start = NULL;
static ServiceCallback       g_on_stop = NULL;
static char                  g_service_name[256] = {0};
static HANDLE                g_stop_event = NULL;

static void win32_log(const char* msg) {
    FILE* f = fopen("C:\\service.log", "a");
    if (f) {
        fprintf(f, "[SERVICE] %s (PID: %lu)\n", msg, GetCurrentProcessId());
        fclose(f);
    }
}

static DWORD WINAPI win32_service_handler_ex(DWORD ctrl, DWORD type, LPVOID data, LPVOID ctx) {
    // Attach to Haxe VM for this thread
    hx::NativeAttach auto_attach;

    switch (ctrl) {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            win32_log("Handler: Stop/Shutdown received.");
            g_status.dwCurrentState = SERVICE_STOP_PENDING;
            SetServiceStatus(g_status_handle, &g_status);
            
            // Execute Haxe stop callback
            if (g_on_stop) g_on_stop();
            
            // Signal the main loop to exit
            if (g_stop_event) SetEvent(g_stop_event);

            g_status.dwCurrentState = SERVICE_STOPPED;
            SetServiceStatus(g_status_handle, &g_status);
            return (DWORD)0; // NO_ERROR
        default:
            return (DWORD)ERROR_CALL_NOT_IMPLEMENTED;
    }
}

static void WINAPI win32_service_main(DWORD argc, LPTSTR* argv) {
    // Attach to Haxe VM for this thread
    hx::NativeAttach auto_attach;
    
    win32_log("ServiceMain: Entered.");
    g_status_handle = RegisterServiceCtrlHandlerExA(g_service_name, win32_service_handler_ex, NULL);
    if (!g_status_handle) {
        win32_log("ServiceMain: FAILED RegisterServiceCtrlHandlerExA");
        return;
    }

    g_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_status.dwServiceSpecificExitCode = 0;
    g_status.dwCheckPoint = 0;
    g_status.dwWaitHint = 0;

    g_status.dwCurrentState = SERVICE_START_PENDING;
    SetServiceStatus(g_status_handle, &g_status);

    // Create event to keep the service thread alive
    g_stop_event = CreateEventA(NULL, TRUE, FALSE, NULL);

    g_status.dwCurrentState = SERVICE_RUNNING;
    g_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    SetServiceStatus(g_status_handle, &g_status);
    win32_log("ServiceMain: RUNNING reported.");

    // Execute Haxe start callback
    if (g_on_start) g_on_start();
    
    // Block until stop event is signaled
    if (g_stop_event) {
        WaitForSingleObject(g_stop_event, INFINITE);
        CloseHandle(g_stop_event);
        g_stop_event = NULL;
    }
    
    win32_log("ServiceMain: Exiting.");
}

#else
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

extern "C" {

int service_notify(const char* status) {
#ifdef _WIN32
    return 0;
#else
    const char* sock_path = getenv("NOTIFY_SOCKET");
    if (!sock_path) return -1;

    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    
    size_t path_len = strlen(sock_path);
    if (path_len >= sizeof(addr.sun_path)) {
        close(fd);
        return -1;
    }

    if (sock_path[0] == '@') {
        addr.sun_path[0] = '\0';
        memcpy(addr.sun_path + 1, sock_path + 1, path_len - 1);
    } else {
        strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path) - 1);
    }

    socklen_t addr_len = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + path_len);
    int res = (int)sendto(fd, status, strlen(status), 0, (struct sockaddr*)&addr, addr_len);
    close(fd);
    return (res >= 0) ? 0 : -1;
#endif
}

int service_is_available() {
#ifdef _WIN32
    HWINSTA hStation = GetProcessWindowStation();
    if (hStation) {
        char name[256];
        DWORD len;
        if (GetUserObjectInformationA(hStation, UOI_NAME, name, sizeof(name), &len)) {
            if (strstr(name, "Service")) return 1;
        }
    }
    return 0;
#else
    return getenv("NOTIFY_SOCKET") != NULL ? 1 : 0;
#endif
}

int service_run(const char* name, ServiceCallback on_start, ServiceCallback on_stop) {
#ifdef _WIN32
    win32_log("service_run: Initializing...");
    strncpy(g_service_name, name, 255);
    g_on_start = on_start;
    g_on_stop = on_stop;

    SERVICE_TABLE_ENTRYA table[] = {
        {(LPSTR)g_service_name, (LPSERVICE_MAIN_FUNCTIONA)win32_service_main},
        {NULL, NULL}
    };
    
    // StartServiceCtrlDispatcherA blocks until the service is stopped.
    // This MUST be called on the main thread of the process.
    if (!StartServiceCtrlDispatcherA(table)) {
        char buf[128];
        snprintf(buf, sizeof(buf), "service_run: FAILED (Error: %lu)", GetLastError());
        win32_log(buf);
        return (int)GetLastError();
    }
    
    return 0;
#else
    if (on_start) on_start();
    return 0;
#endif
}

} // extern "C"
