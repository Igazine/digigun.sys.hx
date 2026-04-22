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
 */
extern "C" {
    int fs_watch_start(const char* path, FSEventCallback callback);
    void fs_watch_stop_all();

    int fs_file_lock(const char* path, int exclusive, int wait);
    void fs_file_unlock(int id);

    int fs_mmap_open(const char* path, int size, int writable);
    void fs_mmap_close(int id);
    int fs_mmap_read(int id, int offset, char* buffer, int length);
    int fs_mmap_write(int id, int offset, const char* buffer, int length);
    void fs_mmap_flush(int id);

    int fs_set_xattr(const char* path, const char* name, const unsigned char* value, int value_len);
    int fs_get_xattr(const char* path, const char* name, unsigned char* buffer, int buffer_len);
    int fs_list_xattrs(const char* path, char* buffer, int buffer_len);
    int fs_remove_xattr(const char* path, const char* name);
}

#endif // FS_NATIVE_H
