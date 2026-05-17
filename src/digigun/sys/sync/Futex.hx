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
     * Blocks the current thread if the value at the given buffer location matches `expectedValue`.
     * @param buffer The NativeBuffer containing the 32-bit integer.
     * @param byteOffset Offset in bytes from the start of the buffer (must be 4-byte aligned).
     * @param expectedValue The value that must be present to trigger the wait.
     * @return True if the wait was successful (awoken or value already changed).
     */
    public static function wait(buffer:digigun.sys.io.NativeBuffer, byteOffset:Int, expectedValue:Int):Bool {
        var ptr:cpp.RawPointer<Int> = cast untyped __cpp__("(int*)((char*){0} + {1})", buffer._getPointer(), byteOffset);
        return Native.wait(ptr, expectedValue) == 0;
    }

    /**
     * Wakes a single thread waiting on the given buffer location.
     */
    public static function wake(buffer:digigun.sys.io.NativeBuffer, byteOffset:Int):Int {
        var ptr:cpp.RawPointer<Int> = cast untyped __cpp__("(int*)((char*){0} + {1})", buffer._getPointer(), byteOffset);
        return Native.wake(ptr);
    }

    /**
     * Wakes all threads waiting on the given buffer location.
     */
    public static function wakeAll(buffer:digigun.sys.io.NativeBuffer, byteOffset:Int):Int {
        var ptr:cpp.RawPointer<Int> = cast untyped __cpp__("(int*)((char*){0} + {1})", buffer._getPointer(), byteOffset);
        return Native.wakeAll(ptr);
    }
}
