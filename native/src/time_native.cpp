#include "time_native.h"

#ifdef _WIN32
#include <windows.h>

extern "C" {

static double get_win_frequency() {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return (double)freq.QuadPart;
}

double time_stamp() {
    static double frequency = get_win_frequency();
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / frequency;
}

long long time_nano_stamp() {
    static double frequency = get_win_frequency();
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (long long)((double)counter.QuadPart * 1000000000.0 / frequency);
}

} // extern "C"

#else
// POSIX Implementation
#include <time.h>

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

extern "C" {

double time_stamp() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

long long time_nano_stamp() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + (long long)ts.tv_nsec;
}

} // extern "C"
#endif
