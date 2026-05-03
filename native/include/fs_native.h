#ifndef FS_NATIVE_H
#define FS_NATIVE_H

/**
 * File System Event Types.
 */
#define FS_EVENT_CREATED  1
#define FS_EVENT_MODIFIED 2
#define FS_EVENT_DELETED  3
#define FS_EVENT_RENAMED  4

/**
 * Callback function signature for file system events.
 */
typedef void (*FSEventCallback)(const char* path, int type, int isDir);

/**
 * Declares file system watching system calls.
 * Uses double for handles to avoid truncation on 64-bit platforms in Haxe.
 */
extern "C" {
    int fs_watch_start(const char* path, FSEventCallback callback);
    void fs_watch_stop_all();

    double fs_file_lock(const char* path, int exclusive, int wait);
    void fs_file_unlock(double id);

    double fs_mmap_open(const char* path, int size, int writable);
    void fs_mmap_close(double id);
    int fs_mmap_read(double id, int offset, char* buffer, int length);
    int fs_mmap_write(double id, int offset, const char* buffer, int length);
    void fs_mmap_flush(double id);

    int fs_set_xattr(const char* path, const char* name, const unsigned char* value, int value_len);
    int fs_get_xattr(const char* path, const char* name, unsigned char* buffer, int buffer_len);
    int fs_list_xattrs(const char* path, char* buffer, int buffer_len);
    int fs_remove_xattr(const char* path, const char* name);
}

#endif // FS_NATIVE_H
