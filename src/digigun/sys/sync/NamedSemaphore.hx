package digigun.sys.sync;

#if cpp
import cpp.Finalizable;
#end

/**
 * Class for managing cross-process named semaphores.
 */
class NamedSemaphore #if cpp extends Finalizable #end {
    private var handle:Float = 0.0;
    private var name:String;

    public function new() {
        #if cpp
        super();
        #end
    }

    /**
     * Opens or creates a named semaphore.
     */
    public function open(name:String, initialValue:Int):Bool {
        #if cpp
        this.name = name;
        this.handle = Native.sem_open(name, initialValue);
        return this.handle != 0.0;
        #else
        return false;
        #end
    }

    /**
     * Waits (locks) the semaphore.
     */
    public function wait():Bool {
        #if cpp
        if (this.handle == 0.0) return false;
        return Native.sem_wait(this.handle) == 0;
        #else
        return false;
        #end
    }

    /**
     * Posts (unlocks) the semaphore.
     */
    public function post():Bool {
        #if cpp
        if (this.handle == 0.0) return false;
        return Native.sem_post(this.handle) == 0;
        #else
        return false;
        #end
    }

    /**
     * Tries to lock the semaphore without blocking.
     */
    public function tryWait():Bool {
        #if cpp
        if (this.handle == 0.0) return false;
        return Native.sem_trywait(this.handle) == 0;
        #else
        return false;
        #end
    }

    /**
     * Closes the semaphore handle.
     */
    public function close():Void {
        #if cpp
        if (this.handle != 0.0) {
            Native.sem_close(this.handle);
            this.handle = 0.0;
        }
        #end
    }

    /**
     * Unlinks (deletes) the named semaphore from the system.
     */
    public function unlink():Void {
        #if cpp
        if (this.name != null) Native.sem_unlink(this.name);
        #end
    }

    #if cpp
    override public function finalize():Void {
        close();
    }
    #end
}
