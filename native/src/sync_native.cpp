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

} // extern "C"
#endif
