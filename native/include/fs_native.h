#ifndef FS_NATIVE_H
#define FS_NATIVE_H

#include <stddef.h>
#include "digigun_export.h"

extern "C" {
    /**
     * File Locking
     */
    DIGIGUN_API long long fs_file_lock(const char* path, int exclusive, int wait);
    DIGIGUN_API void fs_file_unlock(long long id);

    /**
     * File System Watcher
     */
    typedef void (*FSEventCallback)(const char* path, int type, int is_dir);
    DIGIGUN_API int fs_watch_start(const char* path, FSEventCallback callback);
    DIGIGUN_API void fs_watch_stop_all();

    /**
     * Memory Mapped Files
     */
    DIGIGUN_API long long fs_mmap_open(const char* path, int size, int writable);
    DIGIGUN_API void fs_mmap_close(long long id);
    DIGIGUN_API int fs_mmap_read(long long id, int offset, char* buffer, int length);
    DIGIGUN_API int fs_mmap_write(long long id, int offset, const char* buffer, int length);
    DIGIGUN_API void fs_mmap_flush(long long id);

    /**
     * Extended Attributes (xattr / ADS)
     */
    DIGIGUN_API int fs_set_xattr(const char* path, const char* name, const unsigned char* value, int length);
    DIGIGUN_API int fs_get_xattr(const char* path, const char* name, unsigned char* buffer, int length);
    DIGIGUN_API int fs_list_xattrs(const char* path, char* buffer, int length);
    DIGIGUN_API int fs_remove_xattr(const char* path, const char* name);
}

#endif // FS_NATIVE_H
