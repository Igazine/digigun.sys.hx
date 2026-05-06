#include "io_native.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#ifdef __APPLE__
#include <sys/uio.h>
#else
#include <sys/sendfile.h>
#endif
#endif

extern "C" {

long long io_sendfile(long long out_handle, const char* path, long long offset, long long length) {
#ifdef _WIN32
    // Windows implementation using TransmitFile or fallback
    HANDLE hOut = (HANDLE)(size_t)out_handle;
    HANDLE hIn = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hIn == INVALID_HANDLE_VALUE) return -1;

    LARGE_INTEGER liOffset;
    liOffset.QuadPart = offset;
    OVERLAPPED overlapped = {0};
    overlapped.Offset = liOffset.LowPart;
    overlapped.OffsetHigh = liOffset.HighPart;

    BOOL success = TransmitFile((SOCKET)hOut, hIn, (DWORD)length, 0, &overlapped, NULL, 0);
    CloseHandle(hIn);
    return success ? length : -1;
#elif defined(__APPLE__)
    int out_fd = (int)(size_t)out_handle;
    int in_fd = open(path, O_RDONLY);
    if (in_fd < 0) return -1;

    off_t len = length;
    int res = sendfile(in_fd, out_fd, offset, &len, NULL, 0);
    close(in_fd);
    return (res == 0) ? (long long)len : -1;
#else
    int out_fd = (int)(size_t)out_handle;
    int in_fd = open(path, O_RDONLY);
    if (in_fd < 0) return -1;

    off_t off = offset;
    ssize_t res = sendfile(out_fd, in_fd, &off, length);
    close(in_fd);
    return (long long)res;
#endif
}

int io_set_direct_io(long long handle, int enabled) {
#ifdef _WIN32
    // Windows handle flags cannot be changed after creation easily
    return -1;
#elif defined(__APPLE__)
    int fd = (int)(size_t)handle;
    return fcntl(fd, F_NOCACHE, enabled ? 1 : 0);
#else
    // Linux requires O_DIRECT during open()
    return -1;
#endif
}

long long io_open_file(const char* path, int write_mode) {
#ifdef _WIN32
    DWORD access = GENERIC_READ;
    if (write_mode) access |= GENERIC_WRITE;
    HANDLE h = CreateFileA(path, access, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    return (long long)(size_t)h;
#else
    int flags = write_mode ? (O_RDWR | O_CREAT) : O_RDONLY;
    int fd = open(path, flags, 0644);
    return (long long)(size_t)fd;
#endif
}

void io_close_file(long long handle) {
#ifdef _WIN32
    CloseHandle((HANDLE)(size_t)handle);
#else
    close((int)(size_t)handle);
#endif
}

} // extern "C"
