#include "signal_native.h"
#include <signal.h>
#include <map>
#include <mutex>

#ifdef HXCPP_JS_PRIME
#include <hxcpp.h>
#endif

// We need hxcpp.h for hx::NativeAttach and calling into Haxe
#include <hxcpp.h>

namespace digigun {
namespace sys {
namespace signal {

static std::map<int, hx::Object*> g_signal_handlers;
static std::mutex g_signal_mutex;

static void native_signal_handler(int signo) {
    // ATTACH THREAD TO HXCPP
    hx::NativeAttach auto_attach;
    
    std::lock_guard<std::mutex> lock(g_signal_mutex);
    if (g_signal_handlers.count(signo)) {
        hx::Object* cb_obj = g_signal_handlers[signo];
        if (cb_obj) {
            Dynamic cb(cb_obj);
            cb(signo);
        }
    }
}

extern "C" {

int signal_trap(int signo, void* callback) {
    std::lock_guard<std::mutex> lock(g_signal_mutex);
    
    // Store the Haxe callback object
    if (g_signal_handlers.count(signo)) {
        hx::GCRemoveRoot(&g_signal_handlers[signo]);
    }
    
    g_signal_handlers[signo] = (hx::Object*)callback;
    if (g_signal_handlers[signo]) {
        hx::GCAddRoot(&g_signal_handlers[signo]);
    }

    #ifdef _WIN32
    ::signal(signo, native_signal_handler);
    #else
    struct sigaction sa;
    sa.sa_handler = native_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(signo, &sa, NULL) == -1) return -1;
    #endif
    
    return 0;
}

int signal_raise(int signo) {
    return raise(signo);
}

int signal_get_value(const char* name) {
    if (strcmp(name, "INT") == 0) return SIGINT;
    if (strcmp(name, "ILL") == 0) return SIGILL;
    if (strcmp(name, "FPE") == 0) return SIGFPE;
    if (strcmp(name, "SEGV") == 0) return SIGSEGV;
    if (strcmp(name, "TERM") == 0) return SIGTERM;
    if (strcmp(name, "ABRT") == 0) return SIGABRT;
#ifdef _WIN32
    if (strcmp(name, "BREAK") == 0) return SIGBREAK;
#else
    if (strcmp(name, "HUP") == 0) return SIGHUP;
    if (strcmp(name, "QUIT") == 0) return SIGQUIT;
    if (strcmp(name, "TRAP") == 0) return SIGTRAP;
    if (strcmp(name, "KILL") == 0) return SIGKILL;
    if (strcmp(name, "BUS") == 0) return SIGBUS;
    if (strcmp(name, "USR1") == 0) return SIGUSR1;
    if (strcmp(name, "USR2") == 0) return SIGUSR2;
#endif
    return -1;
}

} // extern "C"

} // namespace signal
} // namespace sys
} // namespace digigun
