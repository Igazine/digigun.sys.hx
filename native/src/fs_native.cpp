#include "fs_native.h"
#include <map>
#include <string>
#include <mutex>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/file.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#include <sys/xattr.h>
#else
#include <sys/inotify.h>
#include <limits.h>
#include <sys/xattr.h>
#endif
#endif

// Event types matching Haxe FSEventType.hx
#define FS_EVENT_CREATED  "created"
#define FS_EVENT_MODIFIED "modified"
#define FS_EVENT_DELETED  "deleted"
#define FS_EVENT_RENAMED  "renamed"

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
#ifdef __APPLE__
    FSEventStreamRef stream;
#endif
};
static std::vector<WatcherSession*> g_watchers;
static std::mutex g_watchers_mtx;

#ifdef __APPLE__
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
        const char* type = FS_EVENT_MODIFIED;
        if (eventFlags[i] & kFSEventStreamEventFlagItemCreated) type = FS_EVENT_CREATED;
        else if (eventFlags[i] & kFSEventStreamEventFlagItemRemoved) type = FS_EVENT_DELETED;
        else if (eventFlags[i] & kFSEventStreamEventFlagItemRenamed) type = FS_EVENT_RENAMED;
        else if (eventFlags[i] & kFSEventStreamEventFlagItemModified) type = FS_EVENT_MODIFIED;

        int is_dir = (eventFlags[i] & kFSEventStreamEventFlagItemIsDir) ? 1 : 0;
        session->callback(paths[i], (int)(size_t)type, is_dir); 
    }
}
#endif

int fs_watch_start(const char* path, FSEventCallback callback) {
#ifdef __APPLE__
    WatcherSession* session = new WatcherSession();
    session->path = path;
    session->callback = callback;

    CFStringRef pathCF = CFStringCreateWithCString(NULL, path, kCFStringEncodingUTF8);
    CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void **)&pathCF, 1, NULL);
    
    FSEventStreamContext context = {0, session, NULL, NULL, NULL};
    session->stream = FSEventStreamCreate(NULL, &fs_event_callback, &context, pathsToWatch, kFSEventStreamEventIdSinceNow, 1.0, kFSEventStreamCreateFlagFileEvents);
    
    FSEventStreamScheduleWithRunLoop(session->stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    FSEventStreamStart(session->stream);

    std::lock_guard<std::mutex> lock(g_watchers_mtx);
    g_watchers.push_back(session);
    return 1;
#else
    return 0;
#endif
}

void fs_watch_stop_all() {
#ifdef __APPLE__
    std::lock_guard<std::mutex> lock(g_watchers_mtx);
    for (auto s : g_watchers) {
        FSEventStreamStop(s->stream);
        FSEventStreamInvalidate(s->stream);
        FSEventStreamRelease(s->stream);
        delete s;
    }
    g_watchers.clear();
#endif
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

} // extern "C"
