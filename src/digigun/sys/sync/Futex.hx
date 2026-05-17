package digigun.sys.sync;

@:keep
@:include("sync_native.h")
private extern class Native {
    @:native("sync_futex_wait")
    static function wait(addr:cpp.RawPointer<Int>, expectedValue:Int):Int;

    @:native("sync_futex_wake")
    static function wake(addr:cpp.RawPointer<Int>):Int;

    @:native("sync_futex_wake_all")
    static function wakeAll(addr:cpp.RawPointer<Int>):Int;
}

/**
 * Ultra-low-level synchronization primitive based on Linux Futex and Windows WaitOnAddress.
 * Allows threads to efficiently wait on a memory address until notified.
 */
class Futex {
    /**
     * Blocks the current thread if the value at the given 32-bit pointer matches `expectedValue`.
     * The thread will sleep until `wake()` or `wakeAll()` is called on the same pointer.
     * @param ptr Raw pointer to a 32-bit integer.
     * @param expectedValue The value that must be present to trigger the wait.
     * @return True if the wait was successful (awoken or value already changed).
     */
    public static function wait(ptr:cpp.RawPointer<Int>, expectedValue:Int):Bool {
        return Native.wait(ptr, expectedValue) == 0;
    }

    /**
     * Wakes a single thread waiting on the given pointer.
     * @param ptr Raw pointer to a 32-bit integer.
     * @return Number of threads woken (usually 1 or 0).
     */
    public static function wake(ptr:cpp.RawPointer<Int>):Int {
        return Native.wake(ptr);
    }

    /**
     * Wakes all threads waiting on the given pointer.
     * @param ptr Raw pointer to a 32-bit integer.
     * @return Number of threads woken.
     */
    public static function wakeAll(ptr:cpp.RawPointer<Int>):Int {
        return Native.wakeAll(ptr);
    }
}
