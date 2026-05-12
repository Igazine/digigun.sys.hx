#include "loop_native.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <atomic>

#ifdef __APPLE__
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <mswsock.h>

#else // Linux
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#endif

typedef struct {
#ifdef _WIN32
    OVERLAPPED overlapped;
#endif
    long long fd;
    int op;
    void* buffer;
    int len;
    LoopCallback callback;
    void* user_data;
    int result;
    int bytes;
    bool is_file;
} loop_req_t;

#ifndef _WIN32
typedef struct {
    int loop_fd;
    int self_pipe[2];
    std::queue<loop_req_t*> queue;
    std::mutex mutex;
    std::condition_variable cv;
    std::vector<std::thread> workers;
    std::atomic<bool> stop;
    std::queue<loop_req_t*> completed;
    std::mutex comp_mutex;
} loop_t;

static void posix_worker_thread(loop_t* loop) {
    while (!loop->stop) {
        loop_req_t* req = nullptr;
        {
            std::unique_lock<std::mutex> lock(loop->mutex);
            loop->cv.wait(lock, [loop] { return loop->stop || !loop->queue.empty(); });
            if (loop->stop && loop->queue.empty()) break;
            req = loop->queue.front();
            loop->queue.pop();
        }

        if (req) {
            if (req->op == LOOP_OP_READ) {
                req->bytes = read((int)req->fd, req->buffer, req->len);
                req->result = (req->bytes < 0) ? -errno : 0;
            } else if (req->op == LOOP_OP_WRITE) {
                req->bytes = write((int)req->fd, req->buffer, req->len);
                req->result = (req->bytes < 0) ? -errno : 0;
            }

            {
                std::lock_guard<std::mutex> lock(loop->comp_mutex);
                loop->completed.push(req);
            }
            // Signal loop_poll via self-pipe
            char dummy = 1;
            write(loop->self_pipe[1], &dummy, 1);
        }
    }
}
#endif

extern "C" {

DIGIGUN_API long long loop_create() {
#ifdef _WIN32
    HANDLE port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    return (long long)(size_t)port;
#else
    loop_t* loop = new loop_t();
    loop->stop = false;
#ifdef __APPLE__
    loop->loop_fd = kqueue();
#else
    loop->loop_fd = epoll_create1(0);
#endif
    if (pipe(loop->self_pipe) < 0) {
        delete loop;
        return 0;
    }
    
    // Set self-pipe to non-blocking
    fcntl(loop->self_pipe[0], F_SETFL, O_NONBLOCK);
    
    // Add self-pipe to loop
#ifdef __APPLE__
    struct kevent ev;
    EV_SET(&ev, loop->self_pipe[0], EVFILT_READ, EV_ADD, 0, 0, NULL);
    kevent(loop->loop_fd, &ev, 1, NULL, 0, NULL);
#else
    struct epoll_event ev;
    ev.data.ptr = NULL; // Marker for self-pipe
    ev.events = EPOLLIN;
    epoll_ctl(loop->loop_fd, EPOLL_CTL_ADD, loop->self_pipe[0], &ev);
#endif

    // Start workers
    for (int i = 0; i < 4; i++) {
        loop->workers.emplace_back(posix_worker_thread, loop);
    }
    
    return (long long)(size_t)loop;
#endif
}

DIGIGUN_API void loop_close(long long handle) {
#ifdef _WIN32
    CloseHandle((HANDLE)(size_t)handle);
#else
    loop_t* loop = (loop_t*)(size_t)handle;
    if (!loop) return;
    
    loop->stop = true;
    loop->cv.notify_all();
    for (auto& t : loop->workers) t.join();
    
    close(loop->loop_fd);
    close(loop->self_pipe[0]);
    close(loop->self_pipe[1]);
    delete loop;
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
    req->result = 0;
    req->bytes = 0;

#ifdef _WIN32
    // Associate FD with IOCP
    CreateIoCompletionPort((HANDLE)(size_t)fd, (HANDLE)(size_t)handle, (ULONG_PTR)fd, 0);
    memset(&req->overlapped, 0, sizeof(OVERLAPPED));
    
    BOOL res = FALSE;
    DWORD bytes = 0;
    if (op == LOOP_OP_READ) {
        res = ReadFile((HANDLE)(size_t)fd, buffer, (DWORD)len, &bytes, &req->overlapped);
    } else if (op == LOOP_OP_WRITE) {
        res = WriteFile((HANDLE)(size_t)fd, buffer, (DWORD)len, &bytes, &req->overlapped);
    }

    if (!res && GetLastError() != ERROR_IO_PENDING) {
        free(req);
        return -(int)GetLastError();
    }
    return 0;

#else
    loop_t* loop = (loop_t*)(size_t)handle;
    
    // Check if it's a regular file
    struct stat st;
    fstat((int)fd, &st);
    req->is_file = S_ISREG(st.st_mode);

    if (req->is_file) {
        std::lock_guard<std::mutex> lock(loop->mutex);
        loop->queue.push(req);
        loop->cv.notify_one();
        return 0;
    } else {
        // Socket/Pipe: use direct loop
#ifdef __APPLE__
        struct kevent ev;
        int filter = (op == LOOP_OP_READ || op == LOOP_OP_ACCEPT) ? EVFILT_READ : EVFILT_WRITE;
        EV_SET(&ev, (uintptr_t)fd, filter, EV_ADD | EV_ONESHOT, 0, 0, (void*)req);
        if (kevent(loop->loop_fd, &ev, 1, NULL, 0, NULL) < 0) {
            free(req);
            return -errno;
        }
#else
        struct epoll_event ev;
        ev.data.ptr = req;
        ev.events = ((op == LOOP_OP_READ || op == LOOP_OP_ACCEPT) ? EPOLLIN : EPOLLOUT) | EPOLLET | EPOLLONESHOT;
        if (epoll_ctl(loop->loop_fd, EPOLL_CTL_ADD, (int)fd, &ev) < 0) {
            if (errno == EEXIST) {
                epoll_ctl(loop->loop_fd, EPOLL_CTL_MOD, (int)fd, &ev);
            } else {
                free(req);
                return -errno;
            }
        }
#endif
        return 0;
    }
#endif
}

DIGIGUN_API int loop_poll(long long handle, int timeout_ms) {
    int event_count = 0;

#ifdef _WIN32
    DWORD bytes = 0;
    ULONG_PTR key = 0;
    LPOVERLAPPED overlapped = NULL;
    BOOL res = GetQueuedCompletionStatus((HANDLE)(size_t)handle, &bytes, &key, &overlapped, (DWORD)timeout_ms);
    if (overlapped) {
        loop_req_t* req = (loop_req_t*)overlapped;
        int status = res ? 0 : (int)GetLastError();
        if (req->callback) req->callback(req->user_data, status, (int)bytes);
        free(req);
        event_count++;
    }

#else
    loop_t* loop = (loop_t*)(size_t)handle;
#ifdef __APPLE__
    struct kevent events[64];
    struct timespec ts;
    struct timespec* pts = NULL;
    if (timeout_ms >= 0) {
        ts.tv_sec = timeout_ms / 1000;
        ts.tv_nsec = (timeout_ms % 1000) * 1000000;
        pts = &ts;
    }
    int n = kevent(loop->loop_fd, NULL, 0, events, 64, pts);
#else
    struct epoll_event events[64];
    int n = epoll_wait(loop->loop_fd, events, 64, timeout_ms);
#endif

    for (int i = 0; i < n; i++) {
#ifdef __APPLE__
        void* udata = events[i].udata;
        int fd = (int)events[i].ident;
#else
        void* udata = events[i].data.ptr;
        // Find which fd triggered (epoll doesn't give it directly in data.ptr if we used it for req)
        // But we know udata is either NULL (self-pipe) or loop_req_t*
#endif

        if (udata == NULL) {
            // Self-pipe triggered: drain it and process completed worker tasks
            char buf[64];
            while (read(loop->self_pipe[0], buf, sizeof(buf)) > 0);
            
            std::lock_guard<std::mutex> lock(loop->comp_mutex);
            while (!loop->completed.empty()) {
                loop_req_t* req = loop->completed.front();
                loop->completed.pop();
                if (req->callback) req->callback(req->user_data, req->result, req->bytes);
                free(req);
                event_count++;
            }
        } else {
            loop_req_t* req = (loop_req_t*)udata;
            // Direct I/O (Socket/Pipe)
            if (req->op == LOOP_OP_READ) {
                req->bytes = read((int)req->fd, req->buffer, req->len);
                req->result = (req->bytes < 0) ? -errno : 0;
            } else if (req->op == LOOP_OP_WRITE) {
                req->bytes = write((int)req->fd, req->buffer, req->len);
                req->result = (req->bytes < 0) ? -errno : 0;
            }
            if (req->callback) req->callback(req->user_data, req->result, req->bytes);
            free(req);
            event_count++;
        }
    }
#endif

    return event_count;
}

} // extern "C"
