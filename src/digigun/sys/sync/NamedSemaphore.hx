package digigun.sys.sync;

import digigun.sys.NativeHandle;

/**
 * High-level wrapper for a native named semaphore.
 */
class NamedSemaphore {
    private var handle:NativeHandle;
    private var name:String;

    private function new(handle:NativeHandle, name:String) {
        this.handle = handle;
        this.name = name;
    }

    /**
     * Opens or creates a named semaphore.
     * @param name Unique system-wide name.
     * @param initialValue Initial semaphore count.
     * @return A NamedSemaphore instance if successful, null otherwise.
     */
    public static function open(name:String, initialValue:Int = 1):NamedSemaphore {
        #if cpp
        var val = Native.sem_open(name, initialValue);
        if (val != 0) {
            return new NamedSemaphore(new NativeHandle(val), name);
        }
        #end
        return null;
    }

    /**
     * Decrements the semaphore count. Blocks if count is zero.
     * @return True if successfully decremented.
     */
    public function wait():Bool {
        #if cpp
        if (!this.handle.isValid) return false;
        return Native.sem_wait(this.handle.value) == 0;
        #else
        return false;
        #end
    }

    /**
     * Increments the semaphore count.
     * @return True if successfully incremented.
     */
    public function post():Bool {
        #if cpp
        if (!this.handle.isValid) return false;
        return Native.sem_post(this.handle.value) == 0;
        #else
        return false;
        #end
    }

    /**
     * Attempts to decrement the semaphore count without blocking.
     * @return True if successfully decremented, false if it would have blocked.
     */
    public function tryWait():Bool {
        #if cpp
        if (!this.handle.isValid) return false;
        return Native.sem_trywait(this.handle.value) == 0;
        #else
        return false;
        #end
    }

    /**
     * Closes the local handle to the semaphore.
     */
    public function close():Void {
        #if cpp
        if (this.handle.isValid) {
            Native.sem_close(this.handle.value);
            this.handle = NativeHandle.nullHandle();
        }
        #end
    }

    /**
     * Removes the semaphore from the system.
     */
    public function unlink():Void {
        #if cpp
        Native.sem_unlink(this.name);
        #end
    }
}
