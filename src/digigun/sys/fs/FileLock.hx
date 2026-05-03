package digigun.sys.fs;

/**
 * Provides advisory file locking capabilities.
 */
class FileLock {
    private var handle:Float = -1.0;

    /**
     * Attempts to acquire a lock on the specified file.
     * @param path File to lock.
     * @param exclusive True for exclusive lock, false for shared.
     * @param wait True to wait for the lock, false to fail immediately if busy.
     * @return FileLock instance or null on failure.
     */
    public static function lock(path:String, exclusive:Bool = true, wait:Bool = true):FileLock {
        #if cpp
        var res = Native.file_lock(path, exclusive ? 1 : 0, wait ? 1 : 0);
        if (res != -1.0) {
            var lock = new FileLock();
            lock.handle = res;
            return lock;
        }
        return null;
        #else
        return null;
        #end
    }

    /**
     * Releases the file lock.
     */
    public function unlock():Void {
        #if cpp
        if (handle != -1.0) {
            Native.file_unlock(handle);
            handle = -1.0;
        }
        #end
    }

    private function new() {}
}
