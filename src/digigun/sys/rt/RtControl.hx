package digigun.sys.rt;

#if cpp
@:keep
@:include("rt_native.h")
private extern class Native {
    @:native("rt_mlockall")
    static function mlockall(current:Int, future:Int):Int;

    @:native("rt_munlockall")
    static function munlockall():Int;

    @:native("rt_setup_crash_handler")
    static function setup_crash_handler(reportPath:cpp.ConstCharStar):Int;
}
#end

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

    /**
     * Sets up a native crash handler to capture stack traces and minidumps.
     * When a fatal native error occurs (Segfault, Access Violation), a report
     * is written to the specified path. On Windows, a .dmp file is also generated.
     * @param reportPath Path to the text report file.
     * @return True if successful.
     */
    public static function setupCrashHandler(reportPath:String):Bool {
        #if cpp
        return Native.setup_crash_handler(reportPath) == 0;
        #else
        return false;
        #end
    }
}
