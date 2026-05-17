#include "sync_native.h"
#include <string>
#include <cstring>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

extern "C" {

long long sync_sem_open(const char* name, int initial_value) {
    HANDLE h = CreateSemaphoreA(NULL, initial_value, 2147483647, name);
    return (long long)(size_t)h;
}

int sync_sem_wait(long long handle) {
    return (WaitForSingleObject((HANDLE)(size_t)handle, INFINITE) == WAIT_OBJECT_0) ? 0 : -1;
}

int sync_sem_post(long long handle) {
    return ReleaseSemaphore((HANDLE)(size_t)handle, 1, NULL) ? 0 : -1;
}

int sync_sem_trywait(long long handle) {
    DWORD res = WaitForSingleObject((HANDLE)(size_t)handle, 0);
    if (res == WAIT_OBJECT_0) return 0;
    if (res == WAIT_TIMEOUT) return -2;
    return -1;
}

void sync_sem_close(long long handle) {
    if (handle) CloseHandle((HANDLE)(size_t)handle);
}

void sync_sem_unlink(const char* name) {
}

int sync_futex_wait(int* addr, int expected_value) {
    if (WaitOnAddress(addr, &expected_value, 4, INFINITE)) return 0;
    return -1;
}

int sync_futex_wake(int* addr) {
    WakeByAddressSingle(addr);
    return 1;
}

int sync_futex_wake_all(int* addr) {
    WakeByAddressAll(addr);
    return 1;
}

} // extern "C"

#else
// POSIX Implementation
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

extern "C" {

long long sync_sem_open(const char* name, int initial_value) {
    std::string posix_name = name;
    if (posix_name[0] != '/') posix_name = "/" + posix_name;
    
    sem_t* sem = sem_open(posix_name.c_str(), O_CREAT, 0666, initial_value);
    return (sem == SEM_FAILED) ? 0 : (long long)(size_t)sem;
}

int sync_sem_wait(long long handle) {
    return sem_wait((sem_t*)(size_t)handle);
}

int sync_sem_post(long long handle) {
    return sem_post((sem_t*)(size_t)handle);
}

int sync_sem_trywait(long long handle) {
    if (sem_trywait((sem_t*)(size_t)handle) == 0) return 0;
    if (errno == EAGAIN) return -2;
    return -1;
}

void sync_sem_close(long long handle) {
    if (handle) sem_close((sem_t*)(size_t)handle);
}

void sync_sem_unlink(const char* name) {
    std::string posix_name = name;
    if (posix_name[0] != '/') posix_name = "/" + posix_name;
    sem_unlink(posix_name.c_str());
}

#ifdef __linux__
#include <sys/syscall.h>
#include <unistd.h>
#include <linux/futex.h>

int sync_futex_wait(int* addr, int expected_value) {
    if (syscall(SYS_futex, addr, FUTEX_WAIT_PRIVATE, expected_value, NULL, NULL, 0) == 0) return 0;
    if (errno == EAGAIN) return 0; // Already changed
    return -1;
}

int sync_futex_wake(int* addr) {
    return (int)syscall(SYS_futex, addr, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
}

int sync_futex_wake_all(int* addr) {
    return (int)syscall(SYS_futex, addr, FUTEX_WAKE_PRIVATE, 0x7FFFFFFF, NULL, NULL, 0);
}

#elif defined(__APPLE__)
extern "C" int __ulock_wait(uint32_t op, void *addr, uint64_t val, uint32_t timeout);
extern "C" int __ulock_wake(uint32_t op, void *addr, uint64_t wake_val);

int sync_futex_wait(int* addr, int expected_value) {
    // printf("  [NATIVE] wait addr=%p, exp=%d\n", addr, expected_value);
    int res = __ulock_wait(1, addr, (uint64_t)expected_value, 0);
    if (res >= 0) return 0;
    if (errno == EALREADY || errno == EINTR) return 0;
    // fprintf(stderr, "  [NATIVE] __ulock_wait failed: %d (errno %d)\n", res, errno);
    return -1;
}

int sync_futex_wake(int* addr) {
    // printf("  [NATIVE] wake addr=%p\n", addr);
    // 0x101 = UL_COMPARE_AND_WAIT | ULF_WAKE_ALL
    int res = __ulock_wake(0x101, addr, 0);
    if (res < 0) {
        // fprintf(stderr, "  [NATIVE] __ulock_wake failed: %d (errno %d)\n", res, errno);
        return 0;
    }
    return res;
}

int sync_futex_wake_all(int* addr) {
    int res = __ulock_wake(0x101, addr, 0);
    return (res >= 0) ? res : 0;
}
#else
int sync_futex_wait(int* addr, int expected_value) { return -1; }
int sync_futex_wake(int* addr) { return -1; }
int sync_futex_wake_all(int* addr) { return -1; }
#endif

} // extern "C"
#endif
