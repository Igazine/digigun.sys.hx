#include "loop_native.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#ifdef __APPLE__
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct {
    long long fd;
    int op;
    void* buffer;
    int len;
    LoopCallback callback;
    void* user_data;
} loop_req_t;

#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <mswsock.h>

typedef struct {
    OVERLAPPED overlapped;
    long long fd;
    int op;
    void* buffer;
    int len;
    LoopCallback callback;
    void* user_data;
} loop_req_t;

#else // Linux
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct {
    long long fd;
    int op;
    void* buffer;
    int len;
    LoopCallback callback;
    void* user_data;
} loop_req_t;
#endif

extern "C" {

DIGIGUN_API long long loop_create() {
#ifdef __APPLE__
    return (long long)kqueue();
#elif defined(_WIN32)
    HANDLE port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    return (long long)port;
#else
    return (long long)epoll_create1(0);
#endif
}

DIGIGUN_API void loop_close(long long handle) {
#ifdef _WIN32
    CloseHandle((HANDLE)handle);
#else
    close((int)handle);
#endif
}

DIGIGUN_API int loop_submit(long long handle, long long fd, int op, void* buffer, int len, LoopCallback callback, void* user_data) {
    loop_req_t* req = (loop_req_t*)malloc(sizeof(loop_req_t));
    if (!req) return -1;

    req->fd = fd;
    req->op = op;
    req->buffer = buffer;
    req->len = len;
    req->callback = callback;
    req->user_data = user_data;

#ifdef __APPLE__
    struct kevent ev;
    int filter = (op == LOOP_OP_READ || op == LOOP_OP_ACCEPT) ? EVFILT_READ : EVFILT_WRITE;
    EV_SET(&ev, (uintptr_t)fd, filter, EV_ADD | EV_ONESHOT, 0, 0, (void*)req);
    if (kevent((int)handle, &ev, 1, NULL, 0, NULL) < 0) {
        free(req);
        return -errno;
    }
    return 0;

#elif defined(_WIN32)
    // Associate FD with IOCP
    CreateIoCompletionPort((HANDLE)fd, (HANDLE)handle, (ULONG_PTR)fd, 0);
    
    memset(&req->overlapped, 0, sizeof(OVERLAPPED));
    
    BOOL res = FALSE;
    DWORD bytes = 0;

    if (op == LOOP_OP_READ) {
        res = ReadFile((HANDLE)fd, buffer, (DWORD)len, &bytes, &req->overlapped);
    } else if (op == LOOP_OP_WRITE) {
        res = WriteFile((HANDLE)fd, buffer, (DWORD)len, &bytes, &req->overlapped);
    }

    if (!res && GetLastError() != ERROR_IO_PENDING) {
        free(req);
        return - (int)GetLastError();
    }
    return 0;

#else // Linux
    struct epoll_event ev;
    ev.data.ptr = req;
    ev.events = ((op == LOOP_OP_READ || op == LOOP_OP_ACCEPT) ? EPOLLIN : EPOLLOUT) | EPOLLET | EPOLLONESHOT;
    if (epoll_ctl((int)handle, EPOLL_CTL_ADD, (int)fd, &ev) < 0) {
        if (errno == EEXIST) {
            epoll_ctl((int)handle, EPOLL_CTL_MOD, (int)fd, &ev);
        } else {
            free(req);
            return -errno;
        }
    }
    return 0;
#endif
}

DIGIGUN_API int loop_poll(long long handle, int timeout_ms) {
    int event_count = 0;

#ifdef __APPLE__
    struct kevent events[64];
    struct timespec ts;
    struct timespec* pts = NULL;
    if (timeout_ms >= 0) {
        ts.tv_sec = timeout_ms / 1000;
        ts.tv_nsec = (timeout_ms % 1000) * 1000000;
        pts = &ts;
    }
    int n = kevent((int)handle, NULL, 0, events, 64, pts);
    for (int i = 0; i < n; i++) {
        loop_req_t* req = (loop_req_t*)events[i].udata;
        if (!req) continue;
        int res = 0;
        int bytes = read((int)req->fd, req->buffer, req->len);
        if (bytes < 0) res = -errno;
        if (req->callback) req->callback(req->user_data, res, bytes);
        free(req);
        event_count++;
    }

#elif defined(_WIN32)
    DWORD bytes = 0;
    ULONG_PTR key = 0;
    LPOVERLAPPED overlapped = NULL;
    
    BOOL res = GetQueuedCompletionStatus((HANDLE)handle, &bytes, &key, &overlapped, (DWORD)timeout_ms);
    if (overlapped) {
        loop_req_t* req = (loop_req_t*)overlapped;
        int status = res ? 0 : (int)GetLastError();
        if (req->callback) req->callback(req->user_data, status, (int)bytes);
        free(req);
        event_count++;
    }

#else // Linux
    struct epoll_event events[64];
    int n = epoll_wait((int)handle, events, 64, timeout_ms);
    for (int i = 0; i < n; i++) {
        loop_req_t* req = (loop_req_t*)events[i].data.ptr;
        if (!req) continue;
        int res = 0;
        int bytes = read((int)req->fd, req->buffer, req->len);
        if (bytes < 0) res = -errno;
        if (req->callback) req->callback(req->user_data, res, bytes);
        free(req);
        event_count++;
    }
#endif

    return event_count;
}

} // extern "C"
