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
#include <sys/socket.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/xattr.h>
#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#else
#include <sys/inotify.h>
#endif
#endif

extern "C" {

/**
 * Native File System implementation.
 */

#ifdef _WIN32
struct WatchData {
    HANDLE handle;
    char path_utf8[MAX_PATH];
    FSEventCallback callback;
    bool recursive;
    std::atomic<bool> active;
    uint8_t buffer[4096];
};

static std::vector<WatchData*> g_watches;
static std::mutex g_watch_mutex;

static void win32_watch_thread(WatchData* wd) {
    while (wd->active) {
        DWORD bytesRead = 0;
        if (ReadDirectoryChangesW(wd->handle, wd->buffer, sizeof(wd->buffer), wd->recursive,
                                 FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
                                 &bytesRead, NULL, NULL)) {
            if (bytesRead > 0 && wd->callback && wd->active) {
                FILE_NOTIFY_INFORMATION* fni = (FILE_NOTIFY_INFORMATION*)wd->buffer;
                do {
                    int type = FS_EVENT_MODIFIED;
                    switch(fni->Action) {
                        case FILE_ACTION_ADDED: type = FS_EVENT_CREATED; break;
                        case FILE_ACTION_REMOVED: type = FS_EVENT_DELETED; break;
                        case FILE_ACTION_MODIFIED: type = FS_EVENT_MODIFIED; break;
                        case FILE_ACTION_RENAMED_OLD_NAME: type = FS_EVENT_RENAMED; break;
                        case FILE_ACTION_RENAMED_NEW_NAME: type = FS_EVENT_RENAMED; break;
                    }
                    
                    int nameLen = fni->FileNameLength / sizeof(WCHAR);
                    int size_needed = WideCharToMultiByte(CP_UTF8, 0, fni->FileName, nameLen, NULL, 0, NULL, NULL);
                    std::string filename(size_needed, 0);
                    WideCharToMultiByte(CP_UTF8, 0, fni->FileName, nameLen, &filename[0], size_needed, NULL, NULL);
                    
                    std::string full_path = std::string(wd->path_utf8) + "/" + filename;
                    wd->callback(full_path.c_str(), type, 0);
                    
                    if (fni->NextEntryOffset == 0) break;
                    fni = (FILE_NOTIFY_INFORMATION*)((uint8_t*)fni + fni->NextEntryOffset);
                } while(true);
            }
        } else {
            if (GetLastError() == ERROR_OPERATION_ABORTED) break;
            Sleep(100);
        }
    }
}
#endif

int fs_watch_start(const char* path, FSEventCallback callback) {
#ifdef _WIN32
    std::wstring wpath = utf8_to_wstring(path);
    HANDLE hDir = CreateFileW(wpath.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hDir == INVALID_HANDLE_VALUE) return -1;

    WatchData* wd = new WatchData();
    wd->handle = hDir;
    wd->active = true;
    strncpy(wd->path_utf8, path, MAX_PATH);
    wd->callback = callback;
    wd->recursive = true;
    
    std::lock_guard<std::mutex> lock(g_watch_mutex);
    g_watches.push_back(wd);
    
    std::thread t(win32_watch_thread, wd);
    t.detach();
    return (int)g_watches.size();
#else
    return -1;
#endif
}

void fs_watch_stop_all() {
#ifdef _WIN32
    std::lock_guard<std::mutex> lock(g_watch_mutex);
    for (auto wd : g_watches) {
        wd->active = false;
        CancelIoEx(wd->handle, NULL);
        CloseHandle(wd->handle);
    }
    g_watches.clear();
#endif
}

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

    HANDLE hMap = CreateFileMappingA(hFile, NULL, writable ? PAGE_READWRITE : PAGE_READONLY, 0, size, NULL);
    if (!hMap) { CloseHandle(hFile); return -1.0; }

    void* ptr = MapViewOfFile(hMap, writable ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, size);
    if (!ptr) { CloseHandle(hMap); CloseHandle(hFile); return -1.0; }

    g_mmaps[id].ptr = ptr;
    g_mmaps[id].size = size;
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
    g_mmaps[id].size = size;
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
    if (offset + (size_t)length > g_mmaps[idx].size) length = (int)g_mmaps[idx].size - offset;
    if (length <= 0) return 0;
    memcpy(buffer, (char*)g_mmaps[idx].ptr + offset, length);
    return length;
}

int fs_mmap_write(double id, int offset, const char* buffer, int length) {
    int idx = (int)id;
    if (idx <= 0 || idx >= 1024 || !g_mmaps[idx].active) return -1;
    if (offset + (size_t)length > g_mmaps[idx].size) length = (int)g_mmaps[idx].size - offset;
    if (length <= 0) return 0;
    memcpy((char*)g_mmaps[idx].ptr + offset, buffer, length);
    return length;
}

void fs_mmap_flush(double id) {
    int idx = (int)id;
    if (idx <= 0 || idx >= 1024 || !g_mmaps[idx].active) return;
#ifdef _WIN32
    FlushViewOfFile(g_mmaps[idx].ptr, g_mmaps[idx].size);
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
