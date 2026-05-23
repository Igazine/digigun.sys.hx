#include "fs_native.h"
#include "digigun_alloc.h"
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
#include <sys/stat.h>
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

struct ManagedMmap {
#ifdef _WIN32
    HANDLE hMap;
#else
    int fd;
#endif
    void* ptr;
    int size;
};

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
    char buffer[1024];
    DWORD bytesReturned;
    while (session->active) {
        if (ReadDirectoryChangesW(session->hDir, buffer, sizeof(buffer), session->recursive,
                                  FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
                                  &bytesReturned, NULL, NULL)) {
            FILE_NOTIFY_INFORMATION* event = (FILE_NOTIFY_INFORMATION*)buffer;
            while (event) {
                int type = 0;
                switch (event->Action) {
                    case FILE_ACTION_ADDED: type = EVENT_TYPE_CREATED; break;
                    case FILE_ACTION_REMOVED: type = EVENT_TYPE_DELETED; break;
                    case FILE_ACTION_MODIFIED: type = EVENT_TYPE_MODIFIED; break;
                    case FILE_ACTION_RENAMED_OLD_NAME: type = EVENT_TYPE_RENAMED; break;
                    case FILE_ACTION_RENAMED_NEW_NAME: type = EVENT_TYPE_RENAMED; break;
                }
                
                if (type > 0 && session->callback) {
                    int wlen = event->FileNameLength / sizeof(WCHAR);
                    char filename[MAX_PATH];
                    int len = WideCharToMultiByte(CP_UTF8, 0, event->FileName, wlen, filename, MAX_PATH - 1, NULL, NULL);
                    filename[len] = 0;
                    
                    std::string fullPath = session->path + "\\" + filename;
                    session->callback(fullPath.c_str(), type, 0);
                }
                
                if (event->NextEntryOffset == 0) break;
                event = (FILE_NOTIFY_INFORMATION*)((char*)event + event->NextEntryOffset);
            }
        } else break;
    }
}
#elif defined(__APPLE__)
static void fs_event_callback(ConstFSEventStreamRef streamRef, void* clientCallBackInfo, size_t numEvents, void* eventPaths, const FSEventStreamEventFlags eventFlags[], const FSEventStreamEventId eventIds[]) {
    WatcherSession* session = (WatcherSession*)clientCallBackInfo;
    char** paths = (char**)eventPaths;
    for (size_t i = 0; i < numEvents; i++) {
        int type = EVENT_TYPE_MODIFIED;
        if (eventFlags[i] & kFSEventStreamEventFlagItemCreated) type = EVENT_TYPE_CREATED;
        if (eventFlags[i] & kFSEventStreamEventFlagItemRemoved) type = EVENT_TYPE_DELETED;
        if (eventFlags[i] & kFSEventStreamEventFlagItemRenamed) type = EVENT_TYPE_RENAMED;
        
        bool isDir = (eventFlags[i] & kFSEventStreamEventFlagItemIsDir) != 0;
        if (session->callback) session->callback(paths[i], type, isDir ? 1 : 0);
    }
}
#else
static void linux_add_watch_recursive(int fd, const std::string& path, std::map<int, std::string>& wd_to_path) {
    int wd = inotify_add_watch(fd, path.c_str(), IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVE);
    if (wd >= 0) wd_to_path[wd] = path;

    DIR* dir = opendir(path.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_DIR) {
                std::string name = entry->d_name;
                if (name != "." && name != "..") {
                    linux_add_watch_recursive(fd, path + "/" + name, wd_to_path);
                }
            }
        }
        closedir(dir);
    }
}

static void linux_watcher_thread(WatcherSession* session) {
    char buffer[4096];
    while (session->active) {
        int length = read(session->inotify_fd, buffer, sizeof(buffer));
        if (length < 0) break;
        int i = 0;
        while (i < length) {
            struct inotify_event* event = (struct inotify_event*)&buffer[i];
            if (event->len) {
                int type = 0;
                if (event->mask & IN_CREATE) type = EVENT_TYPE_CREATED;
                else if (event->mask & IN_DELETE) type = EVENT_TYPE_DELETED;
                else if (event->mask & IN_MODIFY) type = EVENT_TYPE_MODIFIED;
                else if (event->mask & IN_MOVE) type = EVENT_TYPE_RENAMED;

                if (type > 0 && session->callback) {
                    std::string fullPath = session->wd_to_path[event->wd] + "/" + event->name;
                    session->callback(fullPath.c_str(), type, (event->mask & IN_ISDIR) ? 1 : 0);
                    
                    if ((event->mask & IN_CREATE) && (event->mask & IN_ISDIR) && session->recursive) {
                        linux_add_watch_recursive(session->inotify_fd, fullPath, session->wd_to_path);
                    }
                }
            }
            i += sizeof(struct inotify_event) + event->len;
        }
    }
}
#endif

int fs_watch_start(const char* path, int recursive, FSEventCallback callback) {
#ifdef _WIN32
    HANDLE hDir = CreateFileA(path, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hDir == INVALID_HANDLE_VALUE) return 0;

    WatcherSession* session = new WatcherSession();
    if (session) digigun::g_active_allocations++;
    session->path = path;
    session->callback = callback;
    session->active = true;
    session->recursive = recursive != 0;
    session->hDir = hDir;
    session->thread = new std::thread(win32_watcher_thread, session);
    if (session->thread) digigun::g_active_allocations++;

    std::lock_guard<std::mutex> lock(g_watchers_mtx);
    g_watchers.push_back(session);
    return 1;
#elif defined(__APPLE__)
    WatcherSession* session = new WatcherSession();
    if (session) digigun::g_active_allocations++;
    session->path = path;
    session->callback = callback;
    session->active = true;
    session->recursive = recursive != 0;

    CFStringRef pathCF = CFStringCreateWithCString(NULL, path, kCFStringEncodingUTF8);
    CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void **)&pathCF, 1, NULL);

    FSEventStreamContext context = {0, session, NULL, NULL, NULL};
    FSEventStreamCreateFlags flags = kFSEventStreamCreateFlagFileEvents;
    if (!session->recursive) flags |= kFSEventStreamCreateFlagNoDefer;

    session->stream = FSEventStreamCreate(NULL, &fs_event_callback, &context, pathsToWatch, kFSEventStreamEventIdSinceNow, 0.1, flags);

    FSEventStreamScheduleWithRunLoop(session->stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    FSEventStreamStart(session->stream);

    CFRelease(pathsToWatch);
    CFRelease(pathCF);

    std::lock_guard<std::mutex> lock(g_watchers_mtx);
    g_watchers.push_back(session);
    return 1;
#else
    int fd = inotify_init();
    if (fd < 0) return 0;

    WatcherSession* session = new WatcherSession();
    if (session) digigun::g_active_allocations++;
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
    if (session->thread) digigun::g_active_allocations++;

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
        digigun::g_active_allocations--;
#elif defined(__APPLE__)
        FSEventStreamStop(s->stream);
        FSEventStreamInvalidate(s->stream);
        FSEventStreamRelease(s->stream);
#else
        close(s->inotify_fd);
        s->thread->detach();
        delete s->thread;
        digigun::g_active_allocations--;
#endif
        delete s;
        digigun::g_active_allocations--;
    }
    g_watchers.clear();
}

long long fs_mmap_open(const char* path, int size, int writable) {
#ifdef _WIN32
    HANDLE hFile = CreateFileA(path, GENERIC_READ | (writable ? GENERIC_WRITE : 0), FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return 0;
    HANDLE hMap = CreateFileMappingA(hFile, NULL, writable ? PAGE_READWRITE : PAGE_READONLY, 0, size, NULL);
    CloseHandle(hFile);
    if (!hMap) return 0;

    void* ptr = MapViewOfFile(hMap, writable ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ, 0, 0, size);
    if (!ptr) { CloseHandle(hMap); return 0; }

    ManagedMmap* m = (ManagedMmap*)malloc(sizeof(ManagedMmap));
    if (m) {
        m->hMap = hMap;
        m->ptr = ptr;
        m->size = size;
        digigun::g_active_allocations++;
    }
    return (long long)(size_t)m;
#else
    int fd = open(path, writable ? O_RDWR : O_RDONLY);
    if (fd < 0) return 0;

    void* ptr = mmap(NULL, size, (writable ? (PROT_READ | PROT_WRITE) : PROT_READ), MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) { close(fd); return 0; }

    ManagedMmap* m = (ManagedMmap*)malloc(sizeof(ManagedMmap));
    if (m) {
        m->fd = fd;
        m->ptr = ptr;
        m->size = size;
        digigun::g_active_allocations++;
    }
    return (long long)(size_t)m;
#endif
}

void fs_mmap_close(long long id) {
    if (!id) return;
    ManagedMmap* m = (ManagedMmap*)(size_t)id;
    if (!m) return;
#ifdef _WIN32
    UnmapViewOfFile(m->ptr);
    CloseHandle(m->hMap);
#else
    munmap(m->ptr, m->size);
    close(m->fd);
#endif
    free(m);
    digigun::g_active_allocations--;
}

int fs_mmap_read(long long id, int offset, char* buffer, int length) {
    if (!id) return -1;
    ManagedMmap* m = (ManagedMmap*)(size_t)id;
    if (offset + length > m->size) length = m->size - offset;
    if (length <= 0) return 0;
    memcpy(buffer, (char*)m->ptr + offset, length);
    return length;
}

int fs_mmap_write(long long id, int offset, const char* buffer, int length) {
    if (!id) return -1;
    ManagedMmap* m = (ManagedMmap*)(size_t)id;
    if (offset + length > m->size) length = m->size - offset;
    if (length <= 0) return 0;
    memcpy((char*)m->ptr + offset, buffer, length);
    return length;
}

void fs_mmap_flush(long long id) {
    if (!id) return;
    ManagedMmap* m = (ManagedMmap*)(size_t)id;
#ifdef _WIN32
    FlushViewOfFile(m->ptr, 0);
#else
    msync(m->ptr, m->size, MS_SYNC);
#endif
}

long long fs_mmap_get_address(long long id) {
    if (!id) return 0;
    ManagedMmap* m = (ManagedMmap*)(size_t)id;
    return (long long)(size_t)m->ptr;
}

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
    return getxattr(path, name, buffer, length, 0, 0);
#else
    return getxattr(path, name, buffer, length);
#endif
}

int fs_stat(const char* path, void* out) {
    fs_stat_t* s = (fs_stat_t*)out;
#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA attr;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &attr)) return (int)GetLastError();
    
    s->size = ((long long)attr.nFileSizeHigh << 32) | attr.nFileSizeLow;
    
    // Simplistic time mapping
    s->mtime = 0; 
    
    s->mode = 0;
    if (attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) s->type = 2;
    else if (attr.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        s->type = 7;
    } else {
        if (s->size == 0) s->type = 3; // Empty
        else s->type = 6; // RegularFile
    }
    return 0;
#else
    struct stat st;
    if (stat(path, &st) != 0) return errno;
    
    s->size = (long long)st.st_size;
    s->atime = (double)st.st_atime;
    s->mtime = (double)st.st_mtime;
    s->ctime = (double)st.st_ctime;
    s->mode = (int)st.st_mode;

    if (S_ISREG(st.st_mode)) {
        if (st.st_size == 0) s->type = 3; // Empty
        else s->type = 6; // RegularFile
    }
    else if (S_ISDIR(st.st_mode)) s->type = 2;
    else if (S_ISLNK(st.st_mode)) s->type = 9;
    else if (S_ISCHR(st.st_mode)) s->type = 1;
    else if (S_ISBLK(st.st_mode)) s->type = 0;
    else if (S_ISFIFO(st.st_mode)) s->type = 4;
    else if (S_ISSOCK(st.st_mode)) s->type = 8;
    else s->type = 5; // Other

    return 0;
#endif
}

int fs_chmod(const char* path, int mode) {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES) return (int)GetLastError();
    
    // Simplistic mapping: if any "write" bit is set, clear READONLY.
    // Unix Write bits: 0222 (owner/group/other write)
    if (mode & 0222) attr &= ~FILE_ATTRIBUTE_READONLY;
    else attr |= FILE_ATTRIBUTE_READONLY;
    
    if (SetFileAttributesA(path, attr)) return 0;
    return (int)GetLastError();
#else
    if (chmod(path, (mode_t)mode) == 0) return 0;
    return errno;
#endif
}

} // extern "C"
