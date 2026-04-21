#include "proc_native.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>

extern "C" {

int proc_set_affinity(int mask) {
    return SetProcessAffinityMask(GetCurrentProcess(), (DWORD_PTR)mask) ? 1 : 0;
}

int proc_get_affinity() {
    DWORD_PTR procMask, sysMask;
    if (GetProcessAffinityMask(GetCurrentProcess(), &procMask, &sysMask)) {
        return (int)procMask;
    }
    return -1;
}

int proc_set_priority(int priority_class) {
    DWORD win_priority;
    switch (priority_class) {
        case 0: win_priority = IDLE_PRIORITY_CLASS; break;
        case 1: win_priority = BELOW_NORMAL_PRIORITY_CLASS; break;
        case 2: win_priority = NORMAL_PRIORITY_CLASS; break;
        case 3: win_priority = ABOVE_NORMAL_PRIORITY_CLASS; break;
        case 4: win_priority = HIGH_PRIORITY_CLASS; break;
        case 5: win_priority = REALTIME_PRIORITY_CLASS; break;
        default: win_priority = NORMAL_PRIORITY_CLASS;
    }
    return SetPriorityClass(GetCurrentProcess(), win_priority) ? 1 : 0;
}

} // extern "C"

#else
// POSIX (Linux/macOS)
#include <unistd.h>
#include <sys/resource.h>

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/thread_policy.h>
#include <mach/mach_init.h>
#else
#include <sched.h>
#endif

extern "C" {

int proc_set_affinity(int mask) {
#ifdef __APPLE__
    thread_affinity_policy_data_t policy = { mask };
    kern_return_t ret = thread_policy_set(
        mach_thread_self(),
        THREAD_AFFINITY_POLICY,
        (thread_policy_t)&policy,
        THREAD_AFFINITY_POLICY_COUNT
    );
    return (ret == KERN_SUCCESS) ? 1 : 0;
#else
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (int i = 0; i < 32; i++) {
        if (mask & (1 << i)) CPU_SET(i, &cpuset);
    }
    return (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) == 0) ? 1 : 0;
#endif
}

int proc_get_affinity() {
#ifdef __APPLE__
    // macOS doesn't have a simple "get" for affinity policy
    return -1;
#else
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    if (sched_getaffinity(0, sizeof(cpu_set_t), &cpuset) == 0) {
        int mask = 0;
        for (int i = 0; i < 32; i++) {
            if (CPU_ISSET(i, &cpuset)) mask |= (1 << i);
        }
        return mask;
    }
    return -1;
#endif
}

int proc_set_priority(int priority_class) {
    int nice_val;
    switch (priority_class) {
        case 0: nice_val = 19; break; // Idle
        case 1: nice_val = 10; break; // Below Normal
        case 2: nice_val = 0;  break; // Normal
        case 3: nice_val = -5; break; // Above Normal
        case 4: nice_val = -15; break; // High
        case 5: nice_val = -20; break; // Realtime (requires root usually)
        default: nice_val = 0;
    }
    // setpriority uses (PRIO_PROCESS, pid, priority)
    return (setpriority(PRIO_PROCESS, 0, nice_val) == 0) ? 1 : 0;
}

} // extern "C"
#endif
