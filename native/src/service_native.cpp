#include "service_native.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <thread>

#ifdef _WIN32
#include <windows.h>

// Global state for Windows SCM
static SERVICE_STATUS_HANDLE g_status_handle = NULL;
static SERVICE_STATUS        g_status = {0};
static ServiceCallback       g_on_start = NULL;
static ServiceCallback       g_on_stop = NULL;
static char                  g_service_name[256] = {0};

static void win32_log(const char* msg) {
    FILE* f = fopen("C:\\service.log", "a");
    if (f) {
        fprintf(f, "[SERVICE] %s (PID: %lu)\n", msg, GetCurrentProcessId());
        fclose(f);
    }
}

static DWORD WINAPI win32_service_handler_ex(DWORD ctrl, DWORD type, LPVOID data, LPVOID ctx) {
    switch (ctrl) {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            win32_log("Handler: Stop/Shutdown received.");
            g_status.dwCurrentState = SERVICE_STOP_PENDING;
            SetServiceStatus(g_status_handle, &g_status);
            if (g_on_stop) g_on_stop();
            g_status.dwCurrentState = SERVICE_STOPPED;
            SetServiceStatus(g_status_handle, &g_status);
            return NO_ERROR;
        default:
            return ERROR_CALL_NOT_IMPLEMENTED;
    }
}

static void WINAPI win32_service_main(DWORD argc, LPTSTR* argv) {
    win32_log("ServiceMain: Entered.");
    g_status_handle = RegisterServiceCtrlHandlerExA(g_service_name, win32_service_handler_ex, NULL);
    if (!g_status_handle) {
        win32_log("ServiceMain: FAILED RegisterServiceCtrlHandlerExA");
        return;
    }

    g_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_status.dwServiceSpecificExitCode = 0;

    g_status.dwCurrentState = SERVICE_START_PENDING;
    SetServiceStatus(g_status_handle, &g_status);

    g_status.dwCurrentState = SERVICE_RUNNING;
    g_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    SetServiceStatus(g_status_handle, &g_status);
    win32_log("ServiceMain: RUNNING reported.");

    // IMPORTANT: In Windows services, the ServiceMain thread SHOULD stay alive
    // but here we are using it to trigger the Haxe callback.
    if (g_on_start) g_on_start();
    
    // Keep this thread alive as long as the service is running
    while (g_status.dwCurrentState == SERVICE_RUNNING) {
        Sleep(500);
    }
    win32_log("ServiceMain: Exiting.");
}

static void win32_dispatcher_thread() {
    win32_log("Dispatcher Thread: Starting...");
    SERVICE_TABLE_ENTRYA table[] = {
        {(LPSTR)g_service_name, (LPSERVICE_MAIN_FUNCTIONA)win32_service_main},
        {NULL, NULL}
    };
    if (!StartServiceCtrlDispatcherA(table)) {
        char buf[128];
        sprintf(buf, "Dispatcher Thread: FAILED (Error: %lu)", GetLastError());
        win32_log(buf);
    }
    win32_log("Dispatcher Thread: Exiting.");
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

    // We MUST run the dispatcher in a background thread because it BLOCKs,
    // and Haxe's main thread must remain responsive or it will be killed by SCM.
    std::thread t(win32_dispatcher_thread);
    t.detach();
    
    // Give dispatcher a moment to start
    Sleep(200);
    return 0;
#else
    if (on_start) on_start();
    return 0;
#endif
}

} // extern "C"
