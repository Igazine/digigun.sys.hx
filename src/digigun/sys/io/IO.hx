package digigun.sys.io;

import digigun.sys.fifo.ISocket;
import haxe.Int64;
import sys.FileSystem;
import sys.io.File;
import digigun.sys.NativeHandle;

/**
 * High-performance I/O utility functions.
 */
class IO {
    /**
     * Transfers data from a file directly to a socket using zero-copy mechanism where supported.
     */
    public static function sendFile(socket:ISocket, path:String, offset:Int64, length:Int64):Int64 {
        if (socket == null || !FileSystem.exists(path)) return -1;

        #if cpp
        var h = socket.getHandle();
        if (!h.isValid) return -1;
        return Native.sendfile(h.value, path, offset, length);
        #else
        return -1;
        #end
    }

    /**
     * Toggles Direct I/O (O_DIRECT / F_NOCACHE) for a raw handle.
     */
    public static function setDirectIO(handle:NativeHandle, enabled:Bool):Bool {
        #if cpp
        if (handle == null || !handle.isValid) return false;
        return Native.set_direct_io(handle.value, enabled ? 1 : 0) == 0;
        #else
        return false;
        #end
    }

    /**
     * Opens a file and returns a raw native handle.
     * This avoids needing to access private Haxe fields.
     */
    public static function openFile(path:String, writeMode:Bool):NativeHandle {
        #if cpp
        return new NativeHandle(Native.open_file(path, writeMode ? 1 : 0));
        #else
        return NativeHandle.nullHandle();
        #end
    }

    /**
     * Closes a raw native handle.
     */
    public static function closeHandle(handle:NativeHandle):Void {
        #if cpp
        if (handle != null && handle.isValid) Native.close_file(handle.value);
        #end
    }
}
