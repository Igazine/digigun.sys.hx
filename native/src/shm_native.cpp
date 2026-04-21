#include "shm_native.h"
#include <string>
#include <cstring>
#include <cstdlib>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

struct ManagedShm {
    HANDLE handle;
    void* ptr;
    int size;
};

extern "C" {

size_t shm_open_segment(const char* name, int size, int writable) {
    DWORD protect = writable ? PAGE_READWRITE : PAGE_READONLY;
    DWORD access = writable ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ;
    
    HANDLE hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, protect, 0, size, name);
    if (!hMap) return 0;

    void* ptr = MapViewOfFile(hMap, access, 0, 0, size);
    if (!ptr) { CloseHandle(hMap); return 0; }

    ManagedShm* m = (ManagedShm*)malloc(sizeof(ManagedShm));
    m->handle = hMap;
    m->ptr = ptr;
    m->size = size;
    return (size_t)m;
}

void shm_close_segment(size_t handle) {
    if (!handle) return;
    ManagedShm* m = (ManagedShm*)handle;
    UnmapViewOfFile(m->ptr);
    CloseHandle(m->handle);
    free(m);
}

int shm_read_segment(size_t handle, int offset, char* buffer, int length) {
    if (!handle) return -1;
    ManagedShm* m = (ManagedShm*)handle;
    if (offset + length > m->size) length = m->size - offset;
    if (length <= 0) return 0;
    memcpy(buffer, (char*)m->ptr + offset, length);
    return length;
}

int shm_write_segment(size_t handle, int offset, const char* buffer, int length) {
    if (!handle) return -1;
    ManagedShm* m = (ManagedShm*)handle;
    if (offset + length > m->size) length = m->size - offset;
    if (length <= 0) return 0;
    memcpy((char*)m->ptr + offset, buffer, length);
    return length;
}

void shm_unlink_segment(const char* name) {
}

} // extern "C"

#else
// POSIX Implementation
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

struct ManagedShm {
    int fd;
    void* ptr;
    int size;
};

extern "C" {

size_t shm_open_segment(const char* name, int size, int writable) {
    std::string posix_name = name;
    if (posix_name[0] != '/') posix_name = "/" + posix_name;

    int fd = shm_open(posix_name.c_str(), (writable ? (O_RDWR | O_CREAT) : O_RDONLY), 0666);
    if (fd == -1) return 0;

    if (writable) ftruncate(fd, size);
    else {
        struct stat st;
        if (fstat(fd, &st) == 0) size = (int)st.st_size;
    }

    void* ptr = mmap(NULL, size, (writable ? (PROT_READ | PROT_WRITE) : PROT_READ), MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) { close(fd); return 0; }

    ManagedShm* m = (ManagedShm*)malloc(sizeof(ManagedShm));
    m->fd = fd;
    m->ptr = ptr;
    m->size = size;
    return (size_t)m;
}

void shm_close_segment(size_t handle) {
    if (!handle) return;
    ManagedShm* m = (ManagedShm*)handle;
    munmap(m->ptr, m->size);
    close(m->fd);
    free(m);
}

int shm_read_segment(size_t handle, int offset, char* buffer, int length) {
    if (!handle) return -1;
    ManagedShm* m = (ManagedShm*)handle;
    if (offset + length > m->size) length = m->size - offset;
    if (length <= 0) return 0;
    memcpy(buffer, (char*)m->ptr + offset, length);
    return length;
}

int shm_write_segment(size_t handle, int offset, const char* buffer, int length) {
    if (!handle) return -1;
    ManagedShm* m = (ManagedShm*)handle;
    if (offset + length > m->size) length = m->size - offset;
    if (length <= 0) return 0;
    memcpy((char*)m->ptr + offset, buffer, length);
    return length;
}

void shm_unlink_segment(const char* name) {
    std::string posix_name = name;
    if (posix_name[0] != '/') posix_name = "/" + posix_name;
    shm_unlink(posix_name.c_str());
}

} // extern "C"
#endif
