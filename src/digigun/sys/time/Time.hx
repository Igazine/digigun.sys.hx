package digigun.sys.time;

import haxe.Int64;

/**
 * Class for high-resolution monotonic timing.
 */
class Time {
    /**
     * Returns a monotonic timestamp in seconds.
     * Unlike Sys.time(), this clock only goes forward and is not affected by system clock adjustments.
     * @return Time in seconds.
     */
    public static function stamp():Float {
        #if cpp
        return Native.stamp();
        #else
        return Date.now().getTime() / 1000.0;
        #end
    }

    /**
     * Returns a high-precision monotonic timestamp in nanoseconds.
     * @return Time in nanoseconds.
     */
    public static function nanoStamp():Int64 {
        #if cpp
        return Native.nano_stamp();
        #else
        return Int64.fromFloat(Date.now().getTime() * 1000000.0);
        #end
    }
}
