package digigun.sys.process;

import cpp.Pointer;

/**
 * Structure representing process information.
 */
typedef ProcessInfo = {
    var pid:Int;
    var ppid:Int;
    var name:String;
    var memoryRss:Float;
    var memoryVirtual:Float;
    var children:Array<ProcessInfo>;
}

/**
 * Class providing static process utility functions.
 */
class Process {
    /**
     * Checks if the current process is running with root/administrator privileges.
     * @return True if running as root/admin, false otherwise.
     */
    public static function isRoot():Bool {
        #if cpp
        return Native.is_root() == 1;
        #else
        return false;
        #end
    }

    /**
     * Retrieves the current limit for open file descriptors (POSIX) or CRT file handles (Windows).
     * @return The current limit, or -1 if not available.
     */
    public static function getFileResourceLimit():Int {
        #if cpp
        return Native.get_file_limit();
        #else
        return -1;
        #end
    }

    /**
     * Sets the limit for open file descriptors (POSIX) or CRT file handles (Windows).
     * @param limit The new limit to set.
     * @return The updated limit if successful, or -1 on error.
     */
    public static function setFileResourceLimit(limit:Int):Int {
        #if cpp
        return Native.set_file_limit(limit);
        #else
        return -1;
        #end
    }

    /**
     * Lists all running processes and their basic telemetry.
     * @return Array of ProcessInfo objects.
     */
    public static function listProcesses():Array<ProcessInfo> {
        #if cpp
        var headRaw = Native.list_all();
        if (headRaw == null) return [];

        var result:Array<ProcessInfo> = [];
        var current = Pointer.fromRaw(headRaw);

        while (current != null && current.raw != null) {
            var ni = current.value;
            result.push({
                pid: ni.pid,
                ppid: ni.ppid,
                name: ni.name.toString(),
                memoryRss: ni.memory_rss,
                memoryVirtual: ni.memory_virtual,
                children: []
            });
            current = Pointer.fromRaw(ni.next);
        }

        Native.free_list(headRaw);
        return result;
        #else
        return [];
        #end
    }

    /**
     * Retrieves a hierarchical process tree starting from the specified root PID.
     * If rootPid is -1, returns the full tree starting from system init/root.
     * @param rootPid The PID to start the tree from.
     * @return A list of root-level processes, each with its children.
     */
    public static function getProcessTree(rootPid:Int = -1):Array<ProcessInfo> {
        var all = listProcesses();
        var map = new Map<Int, ProcessInfo>();
        var roots = new Array<ProcessInfo>();

        for (p in all) map.set(p.pid, p);
        
        for (p in all) {
            var parent = map.get(p.ppid);
            if (parent != null && p.pid != p.ppid) {
                parent.children.push(p);
            } else {
                if (rootPid == -1 || p.pid == rootPid) {
                    roots.push(p);
                }
            }
        }

        if (rootPid != -1) {
            var root = map.get(rootPid);
            return root != null ? [root] : [];
        }

        return roots;
    }

    /**
     * Returns the current process ID.
     * @return The PID.
     */
    public static function getId():Int {
        #if cpp
        return Native.get_id();
        #else
        return -1;
        #end
    }

    /**
     * Forks the current process.
     * On POSIX, this clones the process.
     * On Windows, this emulates a fork by re-launching the current executable.
     * @return 0 in the child process, PID of child in the parent, or -1 on error.
     */
    public static function fork():Int {
        #if cpp
        return Native.fork();
        #else
        return -1;
        #end
    }
}
