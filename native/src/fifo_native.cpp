#include "fifo_native.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <string>
#include <vector>

extern "C" {

int fifo_create(const char* path, int mode) {
    return 0; 
}

size_t fifo_open(const char* path, int write_mode) {
    std::string pipePath = "\\\\.\\pipe\\";
    pipePath += path;

    if (write_mode) {
        HANDLE hPipe = CreateFileA(pipePath.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        return (hPipe == INVALID_HANDLE_VALUE) ? 0 : (size_t)hPipe;
    } else {
        HANDLE hPipe = CreateNamedPipeA(pipePath.c_str(), PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
                                       PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                                       1, 4096, 4096, 0, NULL);
        return (hPipe == INVALID_HANDLE_VALUE) ? 0 : (size_t)hPipe;
    }
}

size_t socket_create() {
    return 0; 
}

int socket_bind(size_t handle, const char* path) {
    return 0;
}

int socket_listen(size_t handle, int backlog) {
    return 0;
}

size_t socket_accept(size_t handle) {
    return 0; 
}

int socket_connect(size_t handle, const char* path) {
    return -1;
}

int fd_read(size_t handle, char* buffer, int length) {
    HANDLE h = (HANDLE)handle;
    DWORD read;
    if (ReadFile(h, buffer, length, &read, NULL)) return (int)read;
    DWORD err = GetLastError();
    if (err == ERROR_NO_DATA || err == ERROR_PIPE_LISTENING) return -2; 
    return -1;
}

int fd_write(size_t handle, const char* buffer, int length) {
    HANDLE h = (HANDLE)handle;
    DWORD written;
    if (WriteFile(h, buffer, length, &written, NULL)) return (int)written;
    DWORD err = GetLastError();
    if (err == ERROR_NO_DATA || err == ERROR_PIPE_LISTENING) return -2; 
    return -1;
}

int fd_set_blocking(size_t handle, int blocking) {
    HANDLE h = (HANDLE)handle;
    DWORD mode = blocking ? PIPE_WAIT : PIPE_NOWAIT;
    return SetNamedPipeHandleState(h, &mode, NULL, NULL) ? 0 : -1;
}

int fd_poll(size_t* handles, int* events, int* revents, int count, int timeout) {
    ULONGLONG start = GetTickCount64();
    int ready = 0;
    while (true) {
        ready = 0;
        for (int i = 0; i < count; i++) {
            HANDLE h = (HANDLE)handles[i];
            revents[i] = 0;
            if (events[i] & 0x0001) { 
                DWORD avail = 0;
                if (PeekNamedPipe(h, NULL, 0, NULL, &avail, NULL) && avail > 0) {
                    revents[i] |= 0x0001;
                    ready++;
                }
            }
            if (events[i] & 0x0004) { 
                revents[i] |= 0x0004;
                ready++;
            }
        }
        if (ready > 0 || timeout == 0) break;
        if (timeout > 0 && (GetTickCount64() - start) >= (ULONGLONG)timeout) break;
        Sleep(10);
    }
    return ready;
}

void fd_close(size_t handle) {
    if (handle) CloseHandle((HANDLE)handle);
}

int fd_get_numeric(size_t handle) {
    return -1; 
}

} // extern "C"

#else
// POSIX Implementation
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <stdint.h>

extern "C" {

int fifo_create(const char* path, int mode) { return mkfifo(path, mode); }

size_t fifo_open(const char* path, int write_mode) {
    int fd = open(path, write_mode ? O_WRONLY : O_RDONLY);
    return (fd == -1) ? 0 : (size_t)fd;
}

size_t socket_create() {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    return (fd == -1) ? 0 : (size_t)fd;
}

int socket_bind(size_t handle, const char* path) {
    int fd = (int)handle;
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    unlink(path);
    return bind(fd, (struct sockaddr*)&addr, sizeof(addr));
}

int socket_listen(size_t handle, int backlog) {
    return listen((int)handle, backlog);
}

size_t socket_accept(size_t handle) {
    int res = accept((int)handle, NULL, NULL);
    if (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) return (size_t)-2;
    return (res == -1) ? 0 : (size_t)res;
}

int socket_connect(size_t handle, const char* path) {
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    int res = connect((int)handle, (struct sockaddr*)&addr, sizeof(addr));
    if (res == -1 && errno == EINPROGRESS) return -2;
    return res;
}

int fd_read(size_t handle, char* buffer, int length) {
    int res = (int)read((int)handle, buffer, length);
    if (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) return -2;
    return res;
}

int fd_write(size_t handle, const char* buffer, int length) {
    int res = (int)write((int)handle, buffer, length);
    if (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) return -2;
    return res;
}

int fd_set_blocking(size_t handle, int blocking) {
    int fd = (int)handle;
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    if (blocking) flags &= ~O_NONBLOCK;
    else flags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags);
}

int fd_poll(size_t* handles, int* events, int* revents, int count, int timeout) {
    struct pollfd* pfds = (struct pollfd*)malloc(sizeof(struct pollfd) * count);
    for (int i = 0; i < count; i++) {
        pfds[i].fd = (int)handles[i];
        pfds[i].events = events[i];
        pfds[i].revents = 0;
    }
    int ret = poll(pfds, count, timeout);
    if (ret > 0) {
        for (int i = 0; i < count; i++) revents[i] = pfds[i].revents;
    }
    free(pfds);
    return ret;
}

void fd_close(size_t handle) {
    int fd = (int)handle;
    if (fd != 0 && fd != -1) close(fd);
}

int fd_get_numeric(size_t handle) {
    return (int)handle;
}

} // extern "C"
#endif
