#include "fs_native.h"
#include <hxcpp.h>
#include <map>
#include <string>
#include <mutex>
#include <vector>
#include <cstring>
#include <thread>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/file.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <dirent.h>
#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#include <sys/xattr.h>
#else
#include <sys/inotify.h>
#include <limits.h>
#include <sys/xattr.h>
#endif
#endif

// Event types matching Watcher.hx _onNativeEvent switch
#define EVENT_TYPE_CREATED  1
#define EVENT_TYPE_MODIFIED 2
#define EVENT_TYPE_DELETED  3
#define EVENT_TYPE_RENAMED  4

extern "C" {

long long fs_file_lock(const char* path, int exclusive, int wait) {
#ifdef _WIN32
    HANDLE hFile = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return 0;
    DWORD flags = exclusive ? LOCKFILE_EXCLUSIVE_LOCK : 0;
    if (!wait) flags |= LOCKFILE_FAIL_IMMEDIATELY;
    OVERLAPPED overlapped = {0};
    if (LockFileEx(hFile, flags, 0, MAXDWORD, MAXDWORD, &overlapped)) return (long long)(size_t)hFile;
    CloseHandle(hFile);
    return 0;
#else
    int fd = open(path, O_RDONLY);
    if (fd < 0) return 0;
    int operation = exclusive ? LOCK_EX : LOCK_SH;
    if (!wait) operation |= LOCK_NB;
    if (flock(fd, operation) == 0) return (long long)(size_t)fd;
    close(fd);
    return 0;
#endif
}

void fs_file_unlock(long long id) {
#ifdef _WIN32
    HANDLE hFile = (HANDLE)(size_t)id;
    OVERLAPPED overlapped = {0};
    UnlockFileEx(hFile, 0, MAXDWORD, MAXDWORD, &overlapped);
    CloseHandle(hFile);
#else
    int fd = (int)(size_t)id;
    flock(fd, LOCK_UN);
    close(fd);
#endif
}

// Global watcher state
struct WatcherSession {
    std::string path;
    FSEventCallback callback;
    bool active;
    bool recursive;
#ifdef _WIN32
    HANDLE hDir;
    std::thread* thread;
#elif defined(__APPLE__)
    FSEventStreamRef stream;
#else
    int inotify_fd;
    std::map<int, std::string> wd_to_path;
    std::thread* thread;
#endif
};
static std::vector<WatcherSession*> g_watchers;
static std::mutex g_watchers_mtx;

#ifdef _WIN32
static void win32_watcher_thread(WatcherSession* session) {
    // Attach this thread to the Haxe GC
    hx::NativeAttach autoAttach;
    
    char buffer[1024 * 16];
    DWORD bytesReturned;
    while (session->active) {
        if (ReadDirectoryChangesW(session->hDir, buffer, sizeof(buffer), session->recursive,
                                  FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
                                  &bytesReturned, NULL, NULL)) {
            FILE_NOTIFY_INFORMATION* event = (FILE_NOTIFY_INFORMATION*)buffer;
            while (event) {
                int type = EVENT_TYPE_MODIFIED;
                switch (event->Action) {
                    case FILE_ACTION_ADDED: type = EVENT_TYPE_CREATED; break;
                    case FILE_ACTION_REMOVED: type = EVENT_TYPE_DELETED; break;
                    case FILE_ACTION_MODIFIED: type = EVENT_TYPE_MODIFIED; break;
                    case FILE_ACTION_RENAMED_OLD_NAME: type = EVENT_TYPE_RENAMED; break;
                    case FILE_ACTION_RENAMED_NEW_NAME: type = EVENT_TYPE_RENAMED; break;
                }

                int nameLen = event->FileNameLength / sizeof(WCHAR);
                char fileName[MAX_PATH];
                WideCharToMultiByte(CP_UTF8, 0, event->FileName, nameLen, fileName, MAX_PATH, NULL, NULL);
                fileName[nameLen] = '\0';

                std::string fullPath = session->path + "\\" + fileName;
                session->callback(fullPath.c_str(), type, 0); // isDir stub for Win32

                if (event->NextEntryOffset == 0) break;
                event = (FILE_NOTIFY_INFORMATION*)((char*)event + event->NextEntryOffset);
            }
        } else break;
    }
}
#elif defined(__APPLE__)
static void fs_event_callback(
    ConstFSEventStreamRef streamRef,
    void *clientCallBackInfo,
    size_t numEvents,
    void *eventPaths,
    const FSEventStreamEventFlags eventFlags[],
    const FSEventStreamEventId eventIds[]) 
{
    WatcherSession* session = (WatcherSession*)clientCallBackInfo;
    char **paths = (char **)eventPaths;

    for (size_t i = 0; i < numEvents; i++) {
        int type = EVENT_TYPE_MODIFIED;
        if (eventFlags[i] & kFSEventStreamEventFlagItemCreated) type = EVENT_TYPE_CREATED;
        else if (eventFlags[i] & kFSEventStreamEventFlagItemRemoved) type = EVENT_TYPE_DELETED;
        else if (eventFlags[i] & kFSEventStreamEventFlagItemRenamed) type = EVENT_TYPE_RENAMED;

        int is_dir = (eventFlags[i] & kFSEventStreamEventFlagItemIsDir) ? 1 : 0;
        session->callback(paths[i], type, is_dir); 
    }
}
#else
static void linux_add_watch_recursive(int fd, std::string path, std::map<int, std::string>& wd_to_path) {
    int wd = inotify_add_watch(fd, path.c_str(), IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVE);
    if (wd >= 0) wd_to_path[wd] = path;

    DIR* dir = opendir(path.c_str());
    if (!dir) return;
    struct dirent* entry;
    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
            linux_add_watch_recursive(fd, path + "/" + entry->d_name, wd_to_path);
        }
    }
    closedir(dir);
}

static void linux_watcher_thread(WatcherSession* session) {
    // Attach this thread to the Haxe GC
    hx::NativeAttach autoAttach;
    
    char buffer[1024 * (sizeof(struct inotify_event) + 16)];
    while (session->active) {
        int length = read(session->inotify_fd, buffer, sizeof(buffer));
        if (length < 0) break;
        int i = 0;
        while (i < length) {
            struct inotify_event* event = (struct inotify_event*)&buffer[i];
            if (event->len) {
                int type = EVENT_TYPE_MODIFIED;
                if (event->mask & IN_CREATE) type = EVENT_TYPE_CREATED;
                else if (event->mask & IN_DELETE) type = EVENT_TYPE_DELETED;
                else if (event->mask & IN_MOVE) type = EVENT_TYPE_RENAMED;
                
                std::string parentPath = session->wd_to_path[event->wd];
                std::string fullPath = parentPath + "/" + event->name;
                
                if (session->recursive && (event->mask & IN_CREATE) && (event->mask & IN_ISDIR)) {
                    linux_add_watch_recursive(session->inotify_fd, fullPath, session->wd_to_path);
                }

                session->callback(fullPath.c_str(), type, (event->mask & IN_ISDIR) ? 1 : 0);
            }
            i += sizeof(struct inotify_event) + event->len;
        }
    }
}
#endif

int fs_watch_start(const char* path, int recursive, FSEventCallback callback) {
#ifdef _WIN32
    HANDLE hDir = CreateFileA(path, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hDir == INVALID_HANDLE_VALUE) return 0;

    WatcherSession* session = new WatcherSession();
    session->path = path;
    session->callback = callback;
    session->active = true;
    session->recursive = recursive != 0;
    session->hDir = hDir;
    session->thread = new std::thread(win32_watcher_thread, session);

    std::lock_guard<std::mutex> lock(g_watchers_mtx);
    g_watchers.push_back(session);
    return 1;
#elif defined(__APPLE__)
    WatcherSession* session = new WatcherSession();
    session->path = path;
    session->callback = callback;
    session->active = true;
    session->recursive = recursive != 0;

    CFStringRef pathCF = CFStringCreateWithCString(NULL, path, kCFStringEncodingUTF8);
    CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void **)&pathCF, 1, NULL);
    
    FSEventStreamContext context = {0, session, NULL, NULL, NULL};
    FSEventStreamCreateFlags flags = kFSEventStreamCreateFlagFileEvents;
    if (!session->recursive) flags |= kFSEventStreamCreateFlagNoDefer; // Best effort non-recursive for Mac

    session->stream = FSEventStreamCreate(NULL, &fs_event_callback, &context, pathsToWatch, kFSEventStreamEventIdSinceNow, 0.1, flags);
    
    FSEventStreamScheduleWithRunLoop(session->stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    FSEventStreamStart(session->stream);

    std::lock_guard<std::mutex> lock(g_watchers_mtx);
    g_watchers.push_back(session);
    return 1;
#else
    int fd = inotify_init();
    if (fd < 0) return 0;

    WatcherSession* session = new WatcherSession();
    session->path = path;
    session->callback = callback;
    session->active = true;
    session->recursive = recursive != 0;
    session->inotify_fd = fd;

    if (session->recursive) {
        linux_add_watch_recursive(fd, path, session->wd_to_path);
    } else {
        int wd = inotify_add_watch(fd, path, IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVE);
        if (wd >= 0) session->wd_to_path[wd] = path;
    }

    session->thread = new std::thread(linux_watcher_thread, session);

    std::lock_guard<std::mutex> lock(g_watchers_mtx);
    g_watchers.push_back(session);
    return 1;
#endif
}

void fs_watch_stop_all() {
    std::lock_guard<std::mutex> lock(g_watchers_mtx);
    for (auto s : g_watchers) {
        s->active = false;
#ifdef _WIN32
        CancelIoEx(s->hDir, NULL);
        CloseHandle(s->hDir);
        s->thread->detach();
        delete s->thread;
#elif defined(__APPLE__)
        FSEventStreamStop(s->stream);
        FSEventStreamInvalidate(s->stream);
        FSEventStreamRelease(s->stream);
#else
        close(s->inotify_fd);
        s->thread->detach();
        delete s->thread;
#endif
        delete s;
    }
    g_watchers.clear();
}

long long fs_mmap_open(const char* path, int size, int writable) {
#ifdef _WIN32
    HANDLE hFile = CreateFileA(path, GENERIC_READ | (writable ? GENERIC_WRITE : 0), FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return 0;
    HANDLE hMap = CreateFileMappingA(hFile, NULL, writable ? PAGE_READWRITE : PAGE_READONLY, 0, size, NULL);
    CloseHandle(hFile);
    return (long long)(size_t)hMap;
#else
    int fd = open(path, writable ? O_RDWR : O_RDONLY);
    return (long long)(size_t)fd;
#endif
}

void fs_mmap_close(long long id) {
#ifdef _WIN32
    CloseHandle((HANDLE)(size_t)id);
#else
    close((int)(size_t)id);
#endif
}

int fs_mmap_read(long long id, int offset, char* buffer, int length) {
#ifdef _WIN32
    void* ptr = MapViewOfFile((HANDLE)(size_t)id, FILE_MAP_READ, 0, offset, length);
    if (!ptr) return -1;
    memcpy(buffer, ptr, length);
    UnmapViewOfFile(ptr);
    return length;
#else
    int fd = (int)(size_t)id;
    void* ptr = mmap(NULL, length, PROT_READ, MAP_SHARED, fd, offset);
    if (ptr == MAP_FAILED) return -1;
    memcpy(buffer, ptr, length);
    munmap(ptr, length);
    return length;
#endif
}

int fs_mmap_write(long long id, int offset, const char* buffer, int length) {
#ifdef _WIN32
    void* ptr = MapViewOfFile((HANDLE)(size_t)id, FILE_MAP_WRITE, 0, offset, length);
    if (!ptr) return -1;
    memcpy(ptr, buffer, length);
    UnmapViewOfFile(ptr);
    return length;
#else
    int fd = (int)(size_t)id;
    void* ptr = mmap(NULL, length, PROT_WRITE, MAP_SHARED, fd, offset);
    if (ptr == MAP_FAILED) return -1;
    memcpy(ptr, buffer, length);
    munmap(ptr, length);
    return length;
#endif
}

void fs_mmap_flush(long long id) {}

int fs_set_xattr(const char* path, const char* name, const unsigned char* value, int length) {
#ifdef _WIN32
    std::string ads_path = path;
    ads_path += ":";
    ads_path += name;
    HANDLE hFile = CreateFileA(ads_path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return -1;
    DWORD written;
    WriteFile(hFile, value, length, &written, NULL);
    CloseHandle(hFile);
    return 0;
#elif defined(__APPLE__)
    return setxattr(path, name, value, length, 0, 0);
#else
    return setxattr(path, name, value, length, 0);
#endif
}

int fs_get_xattr(const char* path, const char* name, unsigned char* buffer, int length) {
#ifdef _WIN32
    std::string ads_path = path;
    ads_path += ":";
    ads_path += name;
    HANDLE hFile = CreateFileA(ads_path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return -1;
    DWORD read;
    ReadFile(hFile, buffer, length, &read, NULL);
    CloseHandle(hFile);
    return (int)read;
#elif defined(__APPLE__)
    return (int)getxattr(path, name, buffer, length, 0, 0);
#else
    return (int)getxattr(path, name, buffer, length);
#endif
}

int fs_list_xattrs(const char* path, char* buffer, int length) {
#ifdef _WIN32
    return -1;
#elif defined(__APPLE__)
    return (int)listxattr(path, buffer, length, 0);
#else
    return (int)listxattr(path, buffer, length);
#endif
}

int fs_remove_xattr(const char* path, const char* name) {
#ifdef _WIN32
    std::string ads_path = path;
    ads_path += ":";
    ads_path += name;
    return DeleteFileA(ads_path.c_str()) ? 0 : -1;
#elif defined(__APPLE__)
    return removexattr(path, name, 0);
#else
    return removexattr(path, name);
#endif
}

int fs_symlink_create(const char* target, const char* linkpath) {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(target);
    DWORD flags = 0;
    if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
        flags |= 0x1; // SYMBOLIC_LINK_FLAG_DIRECTORY
    }
    // 0x2 = SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
    if (CreateSymbolicLinkA(linkpath, target, flags | 0x2)) return 0;
    return (int)GetLastError();
#else
    if (symlink(target, linkpath) == 0) return 0;
    return errno;
#endif
}

int fs_symlink_read(const char* path, char* buffer, int length) {
#ifdef _WIN32
    HANDLE hFile = CreateFileA(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return -1;
    DWORD res = GetFinalPathNameByHandleA(hFile, buffer, length, 0);
    CloseHandle(hFile);
    if (res == 0 || res >= (DWORD)length) return -1;
    if (res > 4 && strncmp(buffer, "\\\\?\\", 4) == 0) {
        memmove(buffer, buffer + 4, res - 3);
        return (int)(res - 4);
    }
    return (int)res;
#else
    ssize_t res = readlink(path, buffer, length);
    if (res < 0) return -1;
    if (res < length) buffer[res] = '\0';
    return (int)res;
#endif
}

} // extern "C"
