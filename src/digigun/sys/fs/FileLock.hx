package digigun.sys.fs;

/**
 * Class for advisory file locking across processes.
 */
class FileLock {
    /**
     * Attempts to acquire a lock on a file.
     * @param path The path to the file to lock.
     * @param exclusive If true, requests an exclusive lock (for writing). 
     *                  If false, requests a shared lock (for reading).
     * @param wait If true, blocks until the lock is acquired.
     *             If false, returns -1 immediately if the lock is busy.
     * @return A lock ID (integer) if successful, or -1 if the lock failed or is busy.
     */
    public static function lock(path:String, exclusive:Bool = true, wait:Bool = false):Int {
        #if cpp
        return Native.file_lock(path, exclusive ? 1 : 0, wait ? 1 : 0);
        #else
        return -1;
        #end
    }

    /**
     * Releases a previously acquired lock.
     * @param id The lock ID returned by `lock()`.
     */
    public static function unlock(id:Int):Void {
        #if cpp
        Native.file_unlock(id);
        #end
    }
}
