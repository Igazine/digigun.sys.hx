package digigun.sys.info;

import cpp.Pointer;

/**
 * Structure representing RAM information.
 */
typedef MemoryInfo = {
    var total:Float;
    var free:Float;
    var used:Float;
}

/**
 * Structure representing Disk space information.
 */
typedef DiskInfo = {
    var total:Float;
    var free:Float;
    var available:Float;
}

/**
 * Class providing hardware and system diagnostics.
 */
class SystemInfo {
    /**
     * Retrieves current RAM statistics in bytes.
     * @return MemoryInfo object.
     */
    public static function getMemoryInfo():MemoryInfo {
        #if cpp
        var total:Float = 0;
        var free:Float = 0;
        var used:Float = 0;
        Native.get_memory(Pointer.addressOf(total), Pointer.addressOf(free), Pointer.addressOf(used));
        return { total: total, free: free, used: used };
        #else
        return { total: -1, free: -1, used: -1 };
        #end
    }

    /**
     * Retrieves the current CPU usage percentage (0.0 to 100.0).
     * Note: On some platforms, the first call may return 0.0 or -1.0 
     * as it needs two samples to calculate usage.
     * @return CPU usage percentage.
     */
    public static function getCpuUsage():Float {
        #if cpp
        return Native.get_cpu_usage();
        #else
        return -1.0;
        #end
    }

    /**
     * Retrieves disk space information for the specified path.
     * @param path The filesystem path (e.g., "/" on Unix, "C:\\" on Windows).
     * @return DiskInfo object.
     */
    public static function getDiskInfo(path:String = "/"):DiskInfo {
        #if cpp
        #if windows
        if (path == "/") path = "C:\\";
        #end
        var total:Float = 0;
        var free:Float = 0;
        var avail:Float = 0;
        Native.get_disk(path, Pointer.addressOf(total), Pointer.addressOf(free), Pointer.addressOf(avail));
        return { total: total, free: free, available: avail };
        #else
        return { total: -1, free: -1, available: -1 };
        #end
    }
}
