package digigun.sys.rt;

/**
 * High-level API for real-time and performance functions.
 */
class RtControl {
    /**
     * Locks the entire address space of the current process into RAM to prevent swapping.
     * Note: This usually requires root or administrative privileges on POSIX systems.
     * @param current Lock currently mapped memory.
     * @param future Lock all future memory mappings.
     * @return True if successful.
     */
    public static function mlockall(current:Bool = true, future:Bool = true):Bool {
        #if cpp
        return Native.mlockall(current ? 1 : 0, future ? 1 : 0) == 0;
        #else
        return false;
        #end
    }

    /**
     * Unlocks all locked memory mappings for the current process.
     * @return True if successful.
     */
    public static function munlockall():Bool {
        #if cpp
        return Native.munlockall() == 0;
        #else
        return false;
        #end
    }
}
