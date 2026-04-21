package digigun.sys.proc;

/**
 * Scheduling priority classes.
 */
enum PriorityClass {
    Idle;
    BelowNormal;
    Normal;
    AboveNormal;
    High;
    Realtime;
}

/**
 * Class for low-level process control (affinity, priority).
 */
class ProcControl {
    /**
     * Sets the CPU affinity mask for the current process.
     * Each bit in the mask represents a CPU core (bit 0 = core 0, bit 1 = core 1, etc).
     * @param mask The bitmask of allowed cores.
     * @return True if successful.
     */
    public static function setAffinity(mask:Int):Bool {
        #if cpp
        return Native.set_affinity(mask) == 1;
        #else
        return false;
        #end
    }

    /**
     * Retrieves the current CPU affinity mask.
     * @return The bitmask of allowed cores, or -1 if not available.
     */
    public static function getAffinity():Int {
        #if cpp
        return Native.get_affinity();
        #else
        return -1;
        #end
    }

    /**
     * Sets the scheduling priority class for the current process.
     * Note: Realtime priority usually requires root/administrative privileges.
     * @param priority The priority class to set.
     * @return True if successful.
     */
    public static function setPriority(priority:PriorityClass):Bool {
        #if cpp
        var val = switch (priority) {
            case Idle: 0;
            case BelowNormal: 1;
            case Normal: 2;
            case AboveNormal: 3;
            case High: 4;
            case Realtime: 5;
        };
        return Native.set_priority(val) == 1;
        #else
        return false;
        #end
    }
}
