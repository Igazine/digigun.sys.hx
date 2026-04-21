#include "io_native.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>

extern "C" {

long long io_sendfile(size_t out_handle, const char* path, long long offset, long long length) {
    HANDLE hOut = (HANDLE)out_handle;
    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return -1;

    char buffer[8192];
    DWORD bytesRead, bytesWritten;
    long long totalSent = 0;
    
    LARGE_INTEGER li; li.QuadPart = offset;
    SetFilePointerEx(hFile, li, NULL, FILE_BEGIN);
    
    while (totalSent < length) {
        DWORD toRead = (DWORD)((length - totalSent) > 8192 ? 8192 : (length - totalSent));
        if (!ReadFile(hFile, buffer, toRead, &bytesRead, NULL) || bytesRead == 0) break;
        if (!WriteFile(hOut, buffer, bytesRead, &bytesWritten, NULL)) break;
        totalSent += bytesWritten;
    }
    
    CloseHandle(hFile);
    return totalSent;
}

int io_set_direct_io(size_t handle, int enabled) {
    return -1; 
}

size_t io_open_file(const char* path, int write_mode) {
    DWORD access = write_mode ? GENERIC_WRITE : GENERIC_READ;
    HANDLE h = CreateFileA(path, access, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    return (h == INVALID_HANDLE_VALUE) ? 0 : (size_t)h;
}

void io_close_file(size_t handle) {
    if (handle) CloseHandle((HANDLE)handle);
}

} // extern "C"

#else
// POSIX Implementation
#include <unistd.h>
#include <stdint.h>

#ifdef __APPLE__
#include <sys/socket.h>
#include <sys/uio.h>
#else
#include <sys/sendfile.h>
#endif

extern "C" {

long long io_sendfile(size_t out_handle, const char* path, long long offset, long long length) {
    int out_fd = (int)out_handle;
    int in_fd = open(path, O_RDONLY);
    if (in_fd == -1) return -1;

    off_t len = (off_t)length;
    long long result = -1;

#ifdef __APPLE__
    if (sendfile(in_fd, out_fd, (off_t)offset, &len, NULL, 0) == 0) {
        result = (long long)len;
    }
#else
    off_t off = (off_t)offset;
    ssize_t sent = sendfile(out_fd, in_fd, &off, (size_t)length);
    if (sent != -1) result = (long long)sent;
#endif

    close(in_fd);
    return result;
}

int io_set_direct_io(size_t handle, int enabled) {
    int fd = (int)handle;
#ifdef __APPLE__
    return fcntl(fd, F_NOCACHE, enabled ? 1 : 0);
#else
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1) return -1;
    if (enabled) flags |= O_DIRECT;
    else flags &= ~O_DIRECT;
    return fcntl(fd, F_SETFL, flags);
#endif
}

size_t io_open_file(const char* path, int write_mode) {
    int fd = open(path, write_mode ? (O_RDWR | O_CREAT) : O_RDONLY, 0666);
    return (fd == -1) ? 0 : (size_t)fd;
}

void io_close_file(size_t handle) {
    int fd = (int)handle;
    if (fd != 0 && fd != -1) close(fd);
}

} // extern "C"
#endif
