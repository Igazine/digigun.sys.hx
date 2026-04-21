#include "sync_native.h"
#include <string>
#include <cstring>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

extern "C" {

size_t sync_sem_open(const char* name, int initial_value) {
    HANDLE h = CreateSemaphoreA(NULL, initial_value, 2147483647, name);
    return (size_t)h;
}

int sync_sem_wait(size_t handle) {
    return (WaitForSingleObject((HANDLE)handle, INFINITE) == WAIT_OBJECT_0) ? 0 : -1;
}

int sync_sem_post(size_t handle) {
    return ReleaseSemaphore((HANDLE)handle, 1, NULL) ? 0 : -1;
}

int sync_sem_trywait(size_t handle) {
    DWORD res = WaitForSingleObject((HANDLE)handle, 0);
    if (res == WAIT_OBJECT_0) return 0;
    if (res == WAIT_TIMEOUT) return -2;
    return -1;
}

void sync_sem_close(size_t handle) {
    if (handle) CloseHandle((HANDLE)handle);
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

extern "C" {

size_t sync_sem_open(const char* name, int initial_value) {
    std::string posix_name = name;
    if (posix_name[0] != '/') posix_name = "/" + posix_name;
    
    sem_t* sem = sem_open(posix_name.c_str(), O_CREAT, 0666, initial_value);
    return (sem == SEM_FAILED) ? 0 : (size_t)sem;
}

int sync_sem_wait(size_t handle) {
    return sem_wait((sem_t*)handle);
}

int sync_sem_post(size_t handle) {
    return sem_post((sem_t*)handle);
}

int sync_sem_trywait(size_t handle) {
    if (sem_trywait((sem_t*)handle) == 0) return 0;
    if (errno == EAGAIN) return -2;
    return -1;
}

void sync_sem_close(size_t handle) {
    if (handle) sem_close((sem_t*)handle);
}

void sync_sem_unlink(const char* name) {
    std::string posix_name = name;
    if (posix_name[0] != '/') posix_name = "/" + posix_name;
    sem_unlink(posix_name.c_str());
}

} // extern "C"
#endif
