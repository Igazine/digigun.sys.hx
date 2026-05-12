#include "rt_native.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>
#include <tchar.h>

static char g_crash_report_path[512] = {0};

static LONG WINAPI win32_exception_filter(EXCEPTION_POINTERS* pep) {
    FILE* f = fopen(g_crash_report_path, "a");
    if (f) {
        fprintf(f, "\n--- NATIVE CRASH REPORT ---\n");
        fprintf(f, "Exception Code: 0x%08X\n", pep->ExceptionRecord->ExceptionCode);
        
        HANDLE process = GetCurrentProcess();
        SymInitialize(process, NULL, TRUE);

        void* stack[64];
        unsigned short frames = CaptureStackBackTrace(0, 64, stack, NULL);
        for (unsigned int i = 0; i < frames; i++) {
            char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
            PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol->MaxNameLen = MAX_SYM_NAME;

            DWORD64 displacement = 0;
            if (SymFromAddr(process, (DWORD64)stack[i], &displacement, symbol)) {
                fprintf(f, "  #%d %s +0x%llx [0x%llx]\n", i, symbol->Name, displacement, symbol->Address);
            } else {
                fprintf(f, "  #%d [unknown symbol] [0x%p]\n", i, stack[i]);
            }
        }
        fclose(f);
    }

    char dump_path[512];
    strncpy(dump_path, g_crash_report_path, 500);
    strcat(dump_path, ".dmp");

    HANDLE hFile = CreateFileA(dump_path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mdei;
        mdei.ThreadId = GetCurrentThreadId();
        mdei.ExceptionPointers = pep;
        mdei.ClientPointers = FALSE;
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &mdei, NULL, NULL);
        CloseHandle(hFile);
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

extern "C" {

int rt_mlockall(int current, int future) {
    HANDLE hProcess = GetCurrentProcess();
    SIZE_T minSize, maxSize;
    if (GetProcessWorkingSetSize(hProcess, &minSize, &maxSize)) {
        if (SetProcessWorkingSetSize(hProcess, minSize * 2, maxSize * 2)) return 0;
    }
    return -1;
}

int rt_munlockall() { return 0; }

int rt_setup_crash_handler(const char* report_path) {
    strncpy(g_crash_report_path, report_path, 511);
    SetUnhandledExceptionFilter(win32_exception_filter);
    return 0;
}

} // extern "C"

#else
#include <sys/mman.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#if defined(__linux__) || defined(__APPLE__)
#include <execinfo.h>
#endif

static char g_crash_report_path[512] = {0};

static void posix_signal_handler(int sig) {
    int fd = open(g_crash_report_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0) {
        const char* msg = "\n--- NATIVE CRASH REPORT ---\nSignal: ";
        if (write(fd, msg, strlen(msg))) {}
        
        char sig_buf[16];
        int len = snprintf(sig_buf, 16, "%d\n", sig);
        if (write(fd, sig_buf, len)) {}
#if defined(__linux__) || defined(__APPLE__)
        void* array[64];
        size_t size = backtrace(array, 64);
        backtrace_symbols_fd(array, size, fd);
#endif
        close(fd);
    }
    _exit(1);
}

extern "C" {

int rt_mlockall(int current, int future) {
    int flags = 0;
    if (current) flags |= MCL_CURRENT;
    if (future) flags |= MCL_FUTURE;
    return mlockall(flags);
}

int rt_munlockall() { return munlockall(); }

int rt_setup_crash_handler(const char* report_path) {
    strncpy(g_crash_report_path, report_path, 511);
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = posix_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND | SA_NODEFER;
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    
    // Use fixed size for alt stack as SIGSTKSZ might not be constant in some glibc versions
    static char alt_stack[65536]; 
    stack_t ss;
    ss.ss_sp = alt_stack;
    ss.ss_size = sizeof(alt_stack);
    ss.ss_flags = 0;
    if (sigaltstack(&ss, NULL) == 0) {
        sa.sa_flags |= SA_ONSTACK;
        sigaction(SIGSEGV, &sa, NULL);
    }
    return 0;
}

} // extern "C"
#endif
