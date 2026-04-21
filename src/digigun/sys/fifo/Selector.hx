package digigun.sys.fifo;

import cpp.NativeArray;
import cpp.RawPointer;
import digigun.sys.NativeHandle;

/**
 * High-level multiplexer to wait for I/O events on multiple FIFOs or Sockets.
 */
class Selector {
    public static function select(read:Array<ICommunicator>, write:Array<ICommunicator>, timeout:Float):{read:Array<ICommunicator>, write:Array<ICommunicator>} {
        #if cpp
        var all = new Map<cpp.SizeT, ICommunicator>();
        var handles = new Array<cpp.SizeT>();
        var events = new Array<Int>();
        var revents = new Array<Int>();

        if (read != null) {
            for (c in read) {
                var h = c.getHandle();
                if (!h.isValid) continue;
                all.set(h.value, c);
                var idx = handles.indexOf(h.value);
                if (idx == -1) {
                    handles.push(h.value);
                    events.push(0x0001); // POLLIN
                } else {
                    events[idx] |= 0x0001;
                }
            }
        }

        if (write != null) {
            for (c in write) {
                var h = c.getHandle();
                if (!h.isValid) continue;
                all.set(h.value, c);
                var idx = handles.indexOf(h.value);
                if (idx == -1) {
                    handles.push(h.value);
                    events.push(0x0004); // POLLOUT
                } else {
                    events[idx] |= 0x0004;
                }
            }
        }

        if (handles.length == 0) return { read: [], write: [] };

        for (i in 0...handles.length) revents.push(0);

        var nativeHandles = NativeArray.address(handles, 0);
        var nativeEvents = NativeArray.address(events, 0);
        var nativeRevents = NativeArray.address(revents, 0);

        var timeoutMs = timeout < 0 ? -1 : Std.int(timeout * 1000);
        var res = Native.fd_poll(cast nativeHandles, cast nativeEvents, cast nativeRevents, handles.length, timeoutMs);

        var readyRead = new Array<ICommunicator>();
        var readyWrite = new Array<ICommunicator>();

        if (res > 0) {
            for (i in 0...handles.length) {
                var hVal = handles[i];
                var rev = revents[i];
                var comm = all.get(hVal);
                if ((rev & 0x0001) != 0 || (rev & 0x0008) != 0) readyRead.push(comm);
                if ((rev & 0x0004) != 0) readyWrite.push(comm);
            }
        }

        return { read: readyRead, write: readyWrite };
        #else
        return { read: [], write: [] };
        #end
    }
}
