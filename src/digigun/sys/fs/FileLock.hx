package digigun.sys.fs;

import digigun.sys.NativeHandle;

/**
 * Provides advisory file locking using flock() or LockFileEx().
 */
class FileLock {
    private var handle:NativeHandle;
    private var path:String;

    private function new(h:NativeHandle, path:String) {
        this.handle = h;
        this.path = path;
    }

    /**
     * Acquires a lock on a file.
     * @param path Path to the file.
     * @param exclusive True for exclusive (write) lock, false for shared (read) lock.
     * @param wait True to block until lock is acquired, false to fail immediately if locked.
     * @return A FileLock instance if successful, null otherwise.
     */
    public static function lock(path:String, exclusive:Bool = true, wait:Bool = true):FileLock {
        #if cpp
        var res = Native.file_lock(path, exclusive ? 1 : 0, wait ? 1 : 0);
        if (res != 0) {
            return new FileLock(new NativeHandle(res), path);
        }
        #end
        return null;
    }

    /**
     * Releases the lock.
     */
    public function unlock():Void {
        #if cpp
        if (this.handle.isValid) {
            Native.file_unlock(this.handle.value);
            this.handle = NativeHandle.nullHandle();
        }
        #end
    }
}
