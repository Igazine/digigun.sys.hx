package digigun.sys.sync;

import digigun.sys.NativeHandle;

/**
 * Class for inter-process synchronization using named semaphores.
 */
class NamedSemaphore {
    private var handle:NativeHandle;
    private var name:String;

    public function new() {
        this.handle = NativeHandle.nullHandle();
    }

    /**
     * Opens or creates a named semaphore.
     */
    public function open(name:String, initialValue:Int = 1):Bool {
        #if cpp
        this.name = name;
        this.handle = new NativeHandle(Native.sem_open(name, initialValue));
        return this.handle.isValid;
        #else
        return false;
        #end
    }

    /**
     * Waits (locks) the semaphore.
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
     * Posts (unlocks) the semaphore.
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
     * Tries to lock the semaphore without blocking.
     */
    public function tryWait():Bool {
        #if cpp
        if (!this.handle.isValid) return false;
        return Native.sem_trywait(this.handle.value) == 0;
        #else
        return false;
        #end
    }

    public function close():Void {
        #if cpp
        if (this.handle.isValid) {
            Native.sem_close(this.handle.value);
            this.handle = NativeHandle.nullHandle();
        }
        #end
    }

    public function unlink():Void {
        #if cpp
        if (this.name != null) Native.sem_unlink(this.name);
        #end
    }
}
