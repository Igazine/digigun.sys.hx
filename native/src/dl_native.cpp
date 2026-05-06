#include "dl_native.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
static char g_win_err[512] = {0};
#else
#include <dlfcn.h>
#endif

extern "C" {

long long dl_open(const char* path) {
#ifdef _WIN32
    HMODULE h = LoadLibraryA(path);
    return (long long)(size_t)h;
#else
    void* h = dlopen(path, RTLD_LAZY | RTLD_GLOBAL);
    return (long long)(size_t)h;
#endif
}

long long dl_get_symbol(long long handle, const char* name) {
#ifdef _WIN32
    FARPROC p = GetProcAddress((HMODULE)(size_t)handle, name);
    return (long long)(size_t)p;
#else
    void* p = dlsym((void*)(size_t)handle, name);
    return (long long)(size_t)p;
#endif
}

void dl_close(long long handle) {
#ifdef _WIN32
    FreeLibrary((HMODULE)(size_t)handle);
#else
    dlclose((void*)(size_t)handle);
#endif
}

const char* dl_get_error() {
#ifdef _WIN32
    DWORD err = GetLastError();
    if (err == 0) return NULL;
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   g_win_err, sizeof(g_win_err), NULL);
    return g_win_err;
#else
    return dlerror();
#endif
}

} // extern "C"
