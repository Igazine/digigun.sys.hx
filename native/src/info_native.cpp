#include "info_native.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>

extern "C" {

void info_get_memory(double* total, double* free, double* used) {
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    if (GlobalMemoryStatusEx(&statex)) {
        *total = (double)statex.ullTotalPhys;
        *free = (double)statex.ullAvailPhys;
        *used = *total - *free;
    } else {
        *total = *free = *used = -1.0;
    }
}

double info_get_cpu_usage() {
    static FILETIME pre_idle, pre_kernel, pre_user;
    FILETIME idle, kernel, user;
    if (GetSystemTimes(&idle, &kernel, &user)) {
        ULARGE_INTEGER i, k, u, pi, pk, pu;
        i.LowPart = idle.dwLowDateTime; i.HighPart = idle.dwHighDateTime;
        k.LowPart = kernel.dwLowDateTime; k.HighPart = kernel.dwHighDateTime;
        u.LowPart = user.dwLowDateTime; u.HighPart = user.dwHighDateTime;
        pi.LowPart = pre_idle.dwLowDateTime; pi.HighPart = pre_idle.dwHighDateTime;
        pk.LowPart = pre_kernel.dwLowDateTime; pk.HighPart = pre_kernel.dwHighDateTime;
        pu.LowPart = pre_user.dwLowDateTime; pu.HighPart = pre_user.dwHighDateTime;

        ULONGLONG idle_diff = i.QuadPart - pi.QuadPart;
        ULONGLONG kernel_diff = k.QuadPart - pk.QuadPart;
        ULONGLONG user_diff = u.QuadPart - pu.QuadPart;
        ULONGLONG total_diff = kernel_diff + user_diff;

        pre_idle = idle; pre_kernel = kernel; pre_user = user;

        if (total_diff == 0) return 0.0;
        return (double)(total_diff - idle_diff) / total_diff * 100.0;
    }
    return -1.0;
}

void info_get_disk(const char* path, double* total, double* free, double* avail) {
    ULARGE_INTEGER free_bytes, total_bytes, total_free_bytes;
    if (GetDiskFreeSpaceExA(path, &free_bytes, &total_bytes, &total_free_bytes)) {
        *total = (double)total_bytes.QuadPart;
        *free = (double)total_free_bytes.QuadPart;
        *avail = (double)free_bytes.QuadPart;
    } else {
        *total = *free = *avail = -1.0;
    }
}

} // extern "C"

#else
// POSIX (Linux/macOS)
#include <sys/statvfs.h>
#include <unistd.h>

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <sys/sysctl.h>
#else
#include <sys/sysinfo.h>
#endif

extern "C" {

void info_get_memory(double* total, double* free, double* used) {
#ifdef __APPLE__
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    vm_statistics64_data_t vm_stats;
    int64_t total_ram;
    size_t size = sizeof(total_ram);
    sysctlbyname("hw.memsize", &total_ram, &size, NULL, 0);
    
    if (host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&vm_stats, &count) == KERN_SUCCESS) {
        *total = (double)total_ram;
        *free = (double)vm_stats.free_count * sysconf(_SC_PAGESIZE);
        *used = *total - *free;
    } else {
        *total = *free = *used = -1.0;
    }
#else
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        *total = (double)si.totalram * si.mem_unit;
        *free = (double)si.freeram * si.mem_unit;
        *used = *total - *free;
    } else {
        *total = *free = *used = -1.0;
    }
#endif
}

double info_get_cpu_usage() {
#ifdef __APPLE__
    static host_cpu_load_info_data_t pre_cpu;
    host_cpu_load_info_data_t cpu;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    
    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&cpu, &count) == KERN_SUCCESS) {
        unsigned long long user = cpu.cpu_ticks[CPU_STATE_USER] - pre_cpu.cpu_ticks[CPU_STATE_USER];
        unsigned long long sys = cpu.cpu_ticks[CPU_STATE_SYSTEM] - pre_cpu.cpu_ticks[CPU_STATE_SYSTEM];
        unsigned long long idle = cpu.cpu_ticks[CPU_STATE_IDLE] - pre_cpu.cpu_ticks[CPU_STATE_IDLE];
        unsigned long long total = user + sys + idle;
        pre_cpu = cpu;
        if (total == 0) return 0.0;
        return (double)(user + sys) / total * 100.0;
    }
#else
    static unsigned long long pre_user, pre_nice, pre_sys, pre_idle;
    unsigned long long user, nice, sys, idle;
    FILE* file = fopen("/proc/stat", "r");
    if (file) {
        char line[256];
        if (fgets(line, sizeof(line), file)) {
            sscanf(line, "cpu %llu %llu %llu %llu", &user, &nice, &sys, &idle);
        }
        fclose(file);
        unsigned long long total = (user + nice + sys + idle) - (pre_user + pre_nice + pre_sys + pre_idle);
        unsigned long long work = (user + nice + sys) - (pre_user + pre_nice + pre_sys);
        pre_user = user; pre_nice = nice; pre_sys = sys; pre_idle = idle;
        if (total == 0) return 0.0;
        return (double)work / total * 100.0;
    }
#endif
    return -1.0;
}

void info_get_disk(const char* path, double* total, double* free, double* avail) {
    struct statvfs vfs;
    if (statvfs(path, &vfs) == 0) {
        *total = (double)vfs.f_blocks * vfs.f_frsize;
        *free = (double)vfs.f_bfree * vfs.f_frsize;
        *avail = (double)vfs.f_bavail * vfs.f_frsize;
    } else {
        *total = *free = *avail = -1.0;
    }
}

} // extern "C"
#endif
