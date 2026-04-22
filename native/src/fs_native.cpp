#include "fs_native.h"
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <cstring>

#include <hxcpp.h>

#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#else
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/xattr.h>
#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#else
#include <sys/inotify.h>
#include <map>
#include <functional>
#endif
#endif

// --- Watcher Implementation ---

static std::atomic<bool> g_watch_active(false);
static FSEventCallback g_current_callback = nullptr;
static std::thread* g_watch_thread = nullptr;

#ifdef _WIN32
static HANDLE g_win_dir_handle = INVALID_HANDLE_VALUE;
static HANDLE g_win_stop_event = INVALID_HANDLE_VALUE;
#endif

static void trigger_callback(const char* path, int type, int isDir) {
    if (g_current_callback) {
        hx::NativeAttach autoAttach;
        g_current_callback(path, type, isDir);
    }
}

#ifdef _WIN32
static void win_watch_thread_proc(std::string path) {
    g_win_dir_handle = CreateFileA(path.c_str(), FILE_LIST_DIRECTORY, 
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
    if (g_win_dir_handle == INVALID_HANDLE_VALUE) return;

    OVERLAPPED overlapped;
    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    char buffer[4096];
    HANDLE waitHandles[2] = { overlapped.hEvent, g_win_stop_event };

    while (g_watch_active) {
        DWORD bytesReturned = 0;
        if (!ReadDirectoryChangesW(g_win_dir_handle, buffer, sizeof(buffer), TRUE, 
                                 FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
                                 &bytesReturned, &overlapped, NULL)) {
            if (GetLastError() != ERROR_IO_PENDING) break;
        }

        DWORD waitRes = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);
        if (waitRes == WAIT_OBJECT_0 + 1) { // Stop event
            CancelIo(g_win_dir_handle);
            break;
        }

        if (waitRes == WAIT_OBJECT_0) { // I/O event
            if (GetOverlappedResult(g_win_dir_handle, &overlapped, &bytesReturned, TRUE)) {
                char* ptr = buffer;
                while (true) {
                    FILE_NOTIFY_INFORMATION* fni = (FILE_NOTIFY_INFORMATION*)ptr;
                    char fileName[MAX_PATH];
                    int nameLen = fni->FileNameLength / sizeof(WCHAR);
                    WideCharToMultiByte(CP_ACP, 0, fni->FileName, nameLen, fileName, MAX_PATH, NULL, NULL);
                    fileName[nameLen] = '\0';

                    int type = 0;
                    switch (fni->Action) {
                        case FILE_ACTION_ADDED: type = FS_EVENT_CREATED; break;
                        case FILE_ACTION_REMOVED: type = FS_EVENT_DELETED; break;
                        case FILE_ACTION_MODIFIED: type = FS_EVENT_MODIFIED; break;
                        case FILE_ACTION_RENAMED_OLD_NAME:
                        case FILE_ACTION_RENAMED_NEW_NAME: type = FS_EVENT_RENAMED; break;
                    }

                    if (type != 0) {
                        std::string fullPath = path + "\\" + fileName;
                        bool isDir = false;
                        if (type != FS_EVENT_DELETED) {
                            DWORD attr = GetFileAttributesA(fullPath.c_str());
                            if (attr != INVALID_FILE_ATTRIBUTES) {
                                isDir = (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
                            }
                        }
                        trigger_callback(fullPath.c_str(), type, isDir ? 1 : 0);
                    }

                    if (fni->NextEntryOffset == 0) break;
                    ptr += fni->NextEntryOffset;
                }
            }
            ResetEvent(overlapped.hEvent);
        } else break;
    }

    CloseHandle(overlapped.hEvent);
    CloseHandle(g_win_dir_handle);
    g_win_dir_handle = INVALID_HANDLE_VALUE;
}
#elif defined(__APPLE__)
static void mac_callback(ConstFSEventStreamRef streamRef, void *clientCallBackInfo, size_t numEvents, void *eventPaths, const FSEventStreamEventFlags eventFlags[], const FSEventStreamEventId eventIds[]) {
    char **paths = (char **)eventPaths;
    for (size_t i = 0; i < numEvents; i++) {
        int type = 0;
        if (eventFlags[i] & kFSEventStreamEventFlagItemCreated) type = FS_EVENT_CREATED;
        else if (eventFlags[i] & kFSEventStreamEventFlagItemRemoved) type = FS_EVENT_DELETED;
        else if (eventFlags[i] & kFSEventStreamEventFlagItemRenamed) type = FS_EVENT_RENAMED;
        else if (eventFlags[i] & kFSEventStreamEventFlagItemModified) type = FS_EVENT_MODIFIED;
        if (type != 0) {
            bool isDir = (eventFlags[i] & kFSEventStreamEventFlagItemIsDir) != 0;
            trigger_callback(paths[i], type, isDir ? 1 : 0);
        }
    }
}
static void mac_watch_thread_proc(std::string path) {
    CFStringRef cfPath = CFStringCreateWithCString(NULL, path.c_str(), kCFStringEncodingUTF8);
    CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void **)&cfPath, 1, NULL);
    FSEventStreamContext context = {0, NULL, NULL, NULL, NULL};
    FSEventStreamRef stream = FSEventStreamCreate(NULL, &mac_callback, &context, pathsToWatch, kFSEventStreamEventIdSinceNow, 0.5, kFSEventStreamCreateFlagFileEvents | kFSEventStreamCreateFlagNoDefer);
    FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    FSEventStreamStart(stream);
    while (g_watch_active) CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.5, false);
    FSEventStreamStop(stream); FSEventStreamInvalidate(stream); FSEventStreamRelease(stream);
    CFRelease(pathsToWatch); CFRelease(cfPath);
}
#else
static void linux_watch_thread_proc(std::string path) {
    int fd = inotify_init(); if (fd < 0) return;
    std::map<int, std::string> watch_map;
    
    std::function<void(const std::string&)> add_recursive = [&](const std::string& p) {
        int wd = inotify_add_watch(fd, p.c_str(), IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVE);
        if (wd != -1) watch_map[wd] = p;
        
        DIR* dir = opendir(p.c_str());
        if (!dir) return;
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_DIR) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
                add_recursive(p + "/" + entry->d_name);
            }
        }
        closedir(dir);
    };

    add_recursive(path);
    char buffer[4096];
    while (g_watch_active) {
        fd_set rfds; FD_ZERO(&rfds); FD_SET(fd, &rfds); struct timeval tv = {0, 500000};
        if (select(fd + 1, &rfds, NULL, NULL, &tv) > 0) {
            ssize_t len = read(fd, buffer, sizeof(buffer)); if (len <= 0) break;
            for (char* ptr = buffer; ptr < buffer + len; ) {
                struct inotify_event* event = (struct inotify_event*)ptr;
                if (event->len > 0) {
                    int type = 0;
                    if (event->mask & IN_CREATE) type = FS_EVENT_CREATED;
                    else if (event->mask & IN_DELETE) type = FS_EVENT_DELETED;
                    else if (event->mask & IN_MODIFY) type = FS_EVENT_MODIFIED;
                    else if (event->mask & (IN_MOVED_FROM | IN_MOVED_TO)) type = FS_EVENT_RENAMED;
                    if (type != 0) {
                        std::string fullPath = watch_map[event->wd] + "/" + event->name;
                        bool isDir = (event->mask & IN_ISDIR) != 0;
                        trigger_callback(fullPath.c_str(), type, isDir ? 1 : 0);
                        if (type == FS_EVENT_CREATED && isDir) {
                            int nwd = inotify_add_watch(fd, fullPath.c_str(), IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVE);
                            if (nwd != -1) watch_map[nwd] = fullPath;
                        }
                    }
                }
                ptr += sizeof(struct inotify_event) + event->len;
            }
        }
    }
    for (std::map<int, std::string>::iterator it = watch_map.begin(); it != watch_map.end(); ++it) {
        inotify_rm_watch(fd, it->first);
    }
    close(fd);
}
#endif

// --- File Locking Implementation ---

struct ManagedLock {
    bool active;
#ifdef _WIN32
    HANDLE handle;
#else
    int fd;
#endif
};

static ManagedLock g_locks[1024];
static int next_lock_slot() {
    for (int i = 1; i < 1024; i++) if (!g_locks[i].active) return i;
    return -1;
}

// --- Memory Mapping Implementation ---

struct ManagedMmap {
    bool active;
    void* ptr;
    int size;
#ifdef _WIN32
    HANDLE file;
    HANDLE mapping;
#else
    int fd;
#endif
};

static ManagedMmap g_mmaps[1024];
static int next_mmap_slot() {
    for (int i = 1; i < 1024; i++) if (!g_mmaps[i].active) return i;
    return -1;
}

extern "C" {

int fs_watch_start(const char* path, FSEventCallback callback) {
    if (g_watch_active) return 0;
    g_current_callback = callback; g_watch_active = true;
#ifdef _WIN32
    g_win_stop_event = CreateEvent(NULL, TRUE, FALSE, NULL);
#endif
    std::string watch_path(path);
#ifdef _WIN32
    g_watch_thread = new std::thread(win_watch_thread_proc, watch_path);
#elif defined(__APPLE__)
    g_watch_thread = new std::thread(mac_watch_thread_proc, watch_path);
#else
    g_watch_thread = new std::thread(linux_watch_thread_proc, watch_path);
#endif
    return 1;
}

void fs_watch_stop_all() {
    if (!g_watch_active) return;
    g_watch_active = false;
#ifdef _WIN32
    if (g_win_stop_event != INVALID_HANDLE_VALUE) {
        SetEvent(g_win_stop_event);
    }
#endif
    if (g_watch_thread) {
        if (g_watch_thread->joinable()) g_watch_thread->join();
        delete g_watch_thread; g_watch_thread = nullptr;
    }
#ifdef _WIN32
    if (g_win_stop_event != INVALID_HANDLE_VALUE) {
        CloseHandle(g_win_stop_event);
        g_win_stop_event = INVALID_HANDLE_VALUE;
    }
#endif
    g_current_callback = nullptr;
}

int fs_file_lock(const char* path, int exclusive, int wait) {
    int slot = next_lock_slot();
    if (slot == -1) return -1;
#ifdef _WIN32
    HANDLE h = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return -1;
    OVERLAPPED ov = {0};
    DWORD flags = exclusive ? LOCKFILE_EXCLUSIVE_LOCK : 0;
    if (!wait) flags |= LOCKFILE_FAIL_IMMEDIATELY;
    if (LockFileEx(h, flags, 0, 0xFFFFFFFF, 0xFFFFFFFF, &ov)) {
        g_locks[slot].active = true; g_locks[slot].handle = h; return slot;
    }
    CloseHandle(h);
#else
    int fd = open(path, O_RDWR | O_CREAT, 0666);
    if (fd == -1) return -1;
    int flags = exclusive ? LOCK_EX : LOCK_SH;
    if (!wait) flags |= LOCK_NB;
    if (flock(fd, flags) == 0) {
        g_locks[slot].active = true; g_locks[slot].fd = fd; return slot;
    }
    close(fd);
#endif
    return -1;
}

void fs_file_unlock(int id) {
    if (id <= 0 || id >= 1024 || !g_locks[id].active) return;
#ifdef _WIN32
    OVERLAPPED ov = {0};
    UnlockFileEx(g_locks[id].handle, 0, 0xFFFFFFFF, 0xFFFFFFFF, &ov);
    CloseHandle(g_locks[id].handle);
#else
    flock(g_locks[id].fd, LOCK_UN); close(g_locks[id].fd);
#endif
    g_locks[id].active = false;
}

int fs_mmap_open(const char* path, int size, int writable) {
    int slot = next_mmap_slot();
    if (slot == -1) return -1;

#ifdef _WIN32
    DWORD access = writable ? GENERIC_READ | GENERIC_WRITE : GENERIC_READ;
    DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE;
    HANDLE hFile = CreateFileA(path, access, share, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return -1;

    if (size == 0) {
        LARGE_INTEGER fs;
        if (GetFileSizeEx(hFile, &fs)) size = (int)fs.QuadPart;
    } else {
        // Ensure file is large enough
        LARGE_INTEGER li; li.QuadPart = size;
        SetFilePointerEx(hFile, li, NULL, FILE_BEGIN);
        SetEndOfFile(hFile);
    }

    DWORD protect = writable ? PAGE_READWRITE : PAGE_READONLY;
    HANDLE hMap = CreateFileMappingA(hFile, NULL, protect, 0, size, NULL);
    if (!hMap) { CloseHandle(hFile); return -1; }

    void* ptr = MapViewOfFile(hMap, writable ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ, 0, 0, size);
    if (!ptr) { CloseHandle(hMap); CloseHandle(hFile); return -1; }

    g_mmaps[slot].active = true; g_mmaps[slot].ptr = ptr; g_mmaps[slot].size = size;
    g_mmaps[slot].file = hFile; g_mmaps[slot].mapping = hMap;
    return slot;
#else
    int fd = open(path, writable ? O_RDWR | O_CREAT : O_RDONLY, 0666);
    if (fd == -1) return -1;

    if (size == 0) {
        struct stat st;
        if (fstat(fd, &st) == 0) size = (int)st.st_size;
    } else if (writable) {
        ftruncate(fd, size);
    }

    void* ptr = mmap(0, size, writable ? PROT_READ | PROT_WRITE : PROT_READ, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) { close(fd); return -1; }

    g_mmaps[slot].active = true; g_mmaps[slot].ptr = ptr; g_mmaps[slot].size = size;
    g_mmaps[slot].fd = fd;
    return slot;
#endif
}

void fs_mmap_close(int id) {
    if (id <= 0 || id >= 1024 || !g_mmaps[id].active) return;
#ifdef _WIN32
    UnmapViewOfFile(g_mmaps[id].ptr);
    CloseHandle(g_mmaps[id].mapping);
    CloseHandle(g_mmaps[id].file);
#else
    munmap(g_mmaps[id].ptr, g_mmaps[id].size);
    close(g_mmaps[id].fd);
#endif
    g_mmaps[id].active = false;
}

int fs_mmap_read(int id, int offset, char* buffer, int length) {
    if (id <= 0 || id >= 1024 || !g_mmaps[id].active) return -1;
    if (offset + length > g_mmaps[id].size) length = g_mmaps[id].size - offset;
    if (length <= 0) return 0;
    memcpy(buffer, (char*)g_mmaps[id].ptr + offset, length);
    return length;
}

int fs_mmap_write(int id, int offset, const char* buffer, int length) {
    if (id <= 0 || id >= 1024 || !g_mmaps[id].active) return -1;
    if (offset + length > g_mmaps[id].size) length = g_mmaps[id].size - offset;
    if (length <= 0) return 0;
    memcpy((char*)g_mmaps[id].ptr + offset, buffer, length);
    return length;
}

void fs_mmap_flush(int id) {
    if (id <= 0 || id >= 1024 || !g_mmaps[id].active) return;
#ifdef _WIN32
    FlushViewOfFile(g_mmaps[id].ptr, g_mmaps[id].size);
#else
    msync(g_mmaps[id].ptr, g_mmaps[id].size, MS_SYNC);
#endif
}

int fs_set_xattr(const char* path, const char* name, const unsigned char* value, int value_len) {
#ifdef _WIN32
    std::string full_path = std::string(path) + ":" + name;
    HANDLE h = CreateFileA(full_path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return -1;
    DWORD written;
    BOOL res = WriteFile(h, value, (DWORD)value_len, &written, NULL);
    CloseHandle(h);
    return res ? 0 : -1;
#else
#ifdef __APPLE__
    return setxattr(path, name, value, (size_t)value_len, 0, 0);
#else
    return setxattr(path, name, value, (size_t)value_len, 0);
#endif
#endif
}

int fs_get_xattr(const char* path, const char* name, unsigned char* buffer, int buffer_len) {
#ifdef _WIN32
    std::string full_path = std::string(path) + ":" + name;
    HANDLE h = CreateFileA(full_path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return -1;
    
    if (buffer == NULL || buffer_len == 0) {
        LARGE_INTEGER size;
        if (GetFileSizeEx(h, &size)) {
            CloseHandle(h);
            return (int)size.QuadPart;
        }
        CloseHandle(h);
        return -1;
    }

    DWORD read;
    BOOL res = ReadFile(h, buffer, (DWORD)buffer_len, &read, NULL);
    CloseHandle(h);
    return res ? (int)read : -1;
#else
#ifdef __APPLE__
    ssize_t res = getxattr(path, name, buffer, (size_t)buffer_len, 0, 0);
#else
    ssize_t res = getxattr(path, name, buffer, (size_t)buffer_len);
#endif
    return (int)res;
#endif
}

int fs_list_xattrs(const char* path, char* buffer, int buffer_len) {
#ifdef _WIN32
    WIN32_FIND_STREAM_DATA data;
    HANDLE h = FindFirstStreamW(std::wstring(std::string(path).begin(), std::string(path).end()).c_str(), FindStreamInfoStandard, &data, 0);
    if (h == INVALID_HANDLE_VALUE) return 0;

    int total_len = 0;
    do {
        // Stream name is in format ":name:$DATA"
        std::wstring ws(data.cStreamName);
        std::string s(ws.begin(), ws.end());
        
        if (s.find(":$DATA") != std::string::npos) {
            size_t first_colon = s.find(':');
            size_t second_colon = s.find(':', first_colon + 1);
            if (second_colon != std::string::npos) {
                std::string stream_name = s.substr(first_colon + 1, second_colon - first_colon - 1);
                if (!stream_name.empty()) {
                    if (buffer != NULL && total_len + stream_name.length() + 1 <= (size_t)buffer_len) {
                        memcpy(buffer + total_len, stream_name.c_str(), stream_name.length() + 1);
                    }
                    total_len += stream_name.length() + 1;
                }
            }
        }
    } while (FindNextStreamW(h, &data));
    FindClose(h);
    return total_len;
#else
#ifdef __APPLE__
    ssize_t res = listxattr(path, buffer, (size_t)buffer_len, 0);
#else
    ssize_t res = listxattr(path, buffer, (size_t)buffer_len);
#endif
    return (int)res;
#endif
}

int fs_remove_xattr(const char* path, const char* name) {
#ifdef _WIN32
    std::string full_path = std::string(path) + ":" + name;
    return DeleteFileA(full_path.c_str()) ? 0 : -1;
#else
#ifdef __APPLE__
    return removexattr(path, name, 0);
#else
    return removexattr(path, name);
#endif
#endif
}

} // extern "C"
