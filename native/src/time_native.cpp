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

void time_sleep_nanos(long long nanos) {
    HANDLE timer = CreateWaitableTimer(NULL, TRUE, NULL);
    LARGE_INTEGER li;
    li.QuadPart = -(nanos / 100); // 100ns units, negative for relative
    SetWaitableTimer(timer, &li, 0, NULL, NULL, FALSE);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
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

void time_sleep_nanos(long long nanos) {
    struct timespec ts;
    ts.tv_sec = nanos / 1000000000LL;
    ts.tv_nsec = nanos % 1000000000LL;
    nanosleep(&ts, NULL);
}

} // extern "C"
#endif
