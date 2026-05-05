#include "fs_native.h"
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <cstring>
#include <map>
#include <functional>

#include <hxcpp.h>

#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

static std::wstring utf8_to_wstring(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

static std::string wstring_to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/xattr.h>
#include <unistd.h>
#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#else
#include <sys/inotify.h>
#endif
#endif

// --- Global Watcher State ---
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
    DWORD bytesReturned;

    HANDLE waitHandles[2] = { overlapped.hEvent, g_win_stop_event };

    while (g_watch_active) {
        if (!ReadDirectoryChangesW(g_win_dir_handle, buffer, sizeof(buffer), TRUE,
                                  FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
                                  &bytesReturned, &overlapped, NULL)) {
            break;
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
                    
                    int nameLen = fni->FileNameLength / sizeof(WCHAR);
                    int size_needed = WideCharToMultiByte(CP_UTF8, 0, fni->FileName, nameLen, NULL, 0, NULL, NULL);
                    std::string filename(size_needed, 0);
                    WideCharToMultiByte(CP_UTF8, 0, fni->FileName, nameLen, &filename[0], size_needed, NULL, NULL);

                    int type = FS_EVENT_MODIFIED;
                    switch (fni->Action) {
                        case FILE_ACTION_ADDED: type = FS_EVENT_CREATED; break;
                        case FILE_ACTION_REMOVED: type = FS_EVENT_DELETED; break;
                        case FILE_ACTION_MODIFIED: type = FS_EVENT_MODIFIED; break;
                        case FILE_ACTION_RENAMED_OLD_NAME: type = FS_EVENT_RENAMED; break;
                        case FILE_ACTION_RENAMED_NEW_NAME: type = FS_EVENT_RENAMED; break;
                    }

                    std::string fullPath = path + "/" + filename;
                    
                    // Simple directory check
                    int isDir = 0;
                    DWORD attr = GetFileAttributesA(fullPath.c_str());
                    if (attr != INVALID_FILE_ATTRIBUTES) isDir = (attr & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;

                    trigger_callback(fullPath.c_str(), type, isDir);

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
    FSEventStreamRef stream = FSEventStreamCreate(NULL, &mac_callback, &context, pathsToWatch, kFSEventStreamEventIdSinceNow, 0.1, kFSEventStreamCreateFlagFileEvents | kFSEventStreamCreateFlagNoDefer);
    FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    FSEventStreamStart(stream);
    while (g_watch_active) CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.1, false);
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
        struct timeval tv = {0, 100000}; // 100ms timeout
        fd_set rfds; FD_ZERO(&rfds); FD_SET(fd, &rfds);
        if (select(fd + 1, &rfds, NULL, NULL, &tv) <= 0) continue;

        int length = read(fd, buffer, sizeof(buffer));
        if (length < 0) break;
        int i = 0;
        while (i < length) {
            struct inotify_event* event = (struct inotify_event*)&buffer[i];
            if (event->len) {
                int type = 0;
                if (event->mask & IN_CREATE) type = FS_EVENT_CREATED;
                else if (event->mask & IN_DELETE) type = FS_EVENT_DELETED;
                else if (event->mask & IN_MODIFY) type = FS_EVENT_MODIFIED;
                else if (event->mask & (IN_MOVED_FROM | IN_MOVED_TO)) type = FS_EVENT_RENAMED;
                
                if (type != 0) {
                    std::string fullPath = watch_map[event->wd] + "/" + event->name;
                    if (event->mask & IN_ISDIR && event->mask & IN_CREATE) add_recursive(fullPath);
                    trigger_callback(fullPath.c_str(), type, (event->mask & IN_ISDIR) ? 1 : 0);
                }
            }
            i += sizeof(struct inotify_event) + event->len;
        }
    }
    for (auto const& [wd, p] : watch_map) inotify_rm_watch(fd, wd);
    close(fd);
}
#endif

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
    SetEvent(g_win_stop_event);
#endif
    if (g_watch_thread) {
        if (g_watch_thread->joinable()) g_watch_thread->join();
        delete g_watch_thread;
        g_watch_thread = nullptr;
    }
#ifdef _WIN32
    if (g_win_stop_event != INVALID_HANDLE_VALUE) {
        CloseHandle(g_win_stop_event);
        g_win_stop_event = INVALID_HANDLE_VALUE;
    }
#endif
}

// File Locking
double fs_file_lock(const char* path, int exclusive, int wait) {
#ifdef _WIN32
    std::wstring wpath = utf8_to_wstring(path);
    HANDLE hFile = CreateFileW(wpath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return -1.0;

    DWORD flags = exclusive ? LOCKFILE_EXCLUSIVE_LOCK : 0;
    if (!wait) flags |= LOCKFILE_FAIL_IMMEDIATELY;

    OVERLAPPED overlapped = {0};
    if (LockFileEx(hFile, flags, 0, MAXDWORD, MAXDWORD, &overlapped)) {
        return (double)(intptr_t)hFile;
    }
    CloseHandle(hFile);
    return -1.0;
#else
    int fd = open(path, O_RDWR);
    if (fd < 0) return -1.0;
    int flags = exclusive ? LOCK_EX : LOCK_SH;
    if (!wait) flags |= LOCK_NB;
    if (flock(fd, flags) == 0) return (double)fd;
    close(fd);
    return -1.0;
#endif
}

void fs_file_unlock(double id) {
    if (id < 0) return;
#ifdef _WIN32
    HANDLE hFile = (HANDLE)(intptr_t)id;
    OVERLAPPED overlapped = {0};
    UnlockFileEx(hFile, 0, MAXDWORD, MAXDWORD, &overlapped);
    CloseHandle(hFile);
#else
    int fd = (int)id;
    flock(fd, LOCK_UN);
    close(fd);
#endif
}

struct MMapData {
    void* ptr;
    size_t size;
    bool active;
#ifdef _WIN32
    HANDLE hFile;
    HANDLE hMap;
#endif
};

static MMapData g_mmaps[1024];

double fs_mmap_open(const char* path, int size, int writable) {
    int id = -1;
    for (int i = 1; i < 1024; i++) {
        if (!g_mmaps[i].active) { id = i; break; }
    }
    if (id == -1) return -1.0;

#ifdef _WIN32
    std::wstring wpath = utf8_to_wstring(path);
    HANDLE hFile = CreateFileW(wpath.c_str(), GENERIC_READ | (writable ? GENERIC_WRITE : 0),
                              FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return -1.0;

    HANDLE hMap = CreateFileMappingA(hFile, NULL, writable ? PAGE_READWRITE : PAGE_READONLY, 0, (DWORD)size, NULL);
    if (!hMap) { CloseHandle(hFile); return -1.0; }

    void* ptr = MapViewOfFile(hMap, writable ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, size);
    if (!ptr) { CloseHandle(hMap); CloseHandle(hFile); return -1.0; }

    g_mmaps[id].ptr = ptr;
    g_mmaps[id].size = (size_t)size;
    g_mmaps[id].hFile = hFile;
    g_mmaps[id].hMap = hMap;
    g_mmaps[id].active = true;
    return (double)id;
#else
    int fd = open(path, writable ? O_RDWR : O_RDONLY);
    if (fd < 0) return -1.0;

    void* ptr = mmap(NULL, size, PROT_READ | (writable ? PROT_WRITE : 0), MAP_SHARED, fd, 0);
    close(fd);
    if (ptr == MAP_FAILED) return -1.0;

    g_mmaps[id].ptr = ptr;
    g_mmaps[id].size = (size_t)size;
    g_mmaps[id].active = true;
    return (double)id;
#endif
}

void fs_mmap_close(double id) {
    int idx = (int)id;
    if (idx <= 0 || idx >= 1024 || !g_mmaps[idx].active) return;
#ifdef _WIN32
    UnmapViewOfFile(g_mmaps[idx].ptr);
    CloseHandle(g_mmaps[idx].hMap);
    CloseHandle(g_mmaps[idx].hFile);
#else
    munmap(g_mmaps[idx].ptr, g_mmaps[idx].size);
#endif
    g_mmaps[idx].active = false;
}

int fs_mmap_read(double id, int offset, char* buffer, int length) {
    int idx = (int)id;
    if (idx <= 0 || idx >= 1024 || !g_mmaps[idx].active) return -1;
    if (offset + (size_t)length > g_mmaps[idx].size) length = (int)(g_mmaps[idx].size - offset);
    if (length <= 0) return 0;
    memcpy(buffer, (char*)g_mmaps[idx].ptr + offset, length);
    return length;
}

int fs_mmap_write(double id, int offset, const char* buffer, int length) {
    int idx = (int)id;
    if (idx <= 0 || idx >= 1024 || !g_mmaps[idx].active) return -1;
    if (offset + (size_t)length > g_mmaps[idx].size) length = (int)(g_mmaps[idx].size - offset);
    if (length <= 0) return 0;
    memcpy((char*)g_mmaps[idx].ptr + offset, buffer, length);
    return length;
}

void fs_mmap_flush(double id) {
    int idx = (int)id;
    if (idx <= 0 || idx >= 1024 || !g_mmaps[idx].active) return;
#ifdef _WIN32
    FlushViewOfFile(g_mmaps[idx].ptr, (SIZE_T)g_mmaps[idx].size);
#else
    msync(g_mmaps[idx].ptr, g_mmaps[idx].size, MS_SYNC);
#endif
}

int fs_set_xattr(const char* path, const char* name, const unsigned char* value, int value_len) {
#ifdef _WIN32
    std::wstring full_path = utf8_to_wstring(path) + L":" + utf8_to_wstring(name);
    HANDLE h = CreateFileW(full_path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
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
    std::wstring full_path = utf8_to_wstring(path) + L":" + utf8_to_wstring(name);
    HANDLE h = CreateFileW(full_path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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
    HANDLE h = FindFirstStreamW(utf8_to_wstring(path).c_str(), FindStreamInfoStandard, &data, 0);
    if (h == INVALID_HANDLE_VALUE) return 0;

    int total_len = 0;
    do {
        std::string s = wstring_to_utf8(data.cStreamName);
        if (s.find(":$DATA") != std::string::npos) {
            size_t first_colon = s.find(':');
            size_t second_colon = s.find(':', first_colon + 1);
            if (second_colon != std::string::npos) {
                std::string stream_name = s.substr(first_colon + 1, second_colon - first_colon - 1);
                if (!stream_name.empty()) {
                    if (buffer != NULL && total_len + (int)stream_name.length() + 1 <= buffer_len) {
                        memcpy(buffer + total_len, stream_name.c_str(), stream_name.length() + 1);
                    }
                    total_len += (int)stream_name.length() + 1;
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
    std::wstring full_path = utf8_to_wstring(path) + L":" + utf8_to_wstring(name);
    return DeleteFileW(full_path.c_str()) ? 0 : -1;
#else
#ifdef __APPLE__
    return removexattr(path, name, 0);
#else
    return removexattr(path, name);
#endif
#endif
}

} // extern "C"
