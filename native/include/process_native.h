#ifndef PROCESS_NATIVE_H
#define PROCESS_NATIVE_H

/**
 * Native process information structure.
 */
struct NativeProcessInfo {
    int pid;
    int ppid;
    char name[256];
    double memory_rss;
    double memory_virtual;
    NativeProcessInfo* next;
};

/**
 * Declares process system calls.
 */
extern "C" {
    /**
     * Checks if the process is elevated (root/admin).
     */
    int process_is_root();

    /**
     * Gets the file resource limit.
     */
    int process_get_file_limit();

    /**
     * Sets the file resource limit.
     */
    int process_set_file_limit(int limit);

    /**
     * Lists all running processes.
     */
    NativeProcessInfo* process_list_all();

    /**
     * Frees the process info list.
     */
    void process_free_list(NativeProcessInfo* list);

    /**
     * Gets the current process ID.
     */
    int process_get_id();

    /**
     * Forks the current process.
     */
    int process_fork();
}

#endif // PROCESS_NATIVE_H
