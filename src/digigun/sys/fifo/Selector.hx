package digigun.sys.fifo;

import cpp.NativeArray;
import cpp.RawPointer;
import digigun.sys.NativeHandle;

/**
 * High-level multiplexer to wait for I/O events on multiple FIFOs or Sockets.
 */
@:cppFileCode("#include <fifo_native.h>")
class Selector {
    public static function select(read:Array<ICommunicator>, write:Array<ICommunicator>, timeout:Float):{read:Array<ICommunicator>, write:Array<ICommunicator>} {
        #if cpp
        var all = new Map<haxe.Int64, ICommunicator>();
        var handles = new Array<haxe.Int64>();
        var events = new Array<Int>();
        var revents = new Array<Int>();

        if (read != null) {
            for (c in read) {
                var h = c.getHandle();
                if (!h.isValid) continue;
                var hVal:haxe.Int64 = cast h.value;
                all.set(hVal, c);
                var idx = -1;
                for (i in 0...handles.length) {
                    if (handles[i] == hVal) {
                        idx = i;
                        break;
                    }
                }
                if (idx == -1) {
                    handles.push(hVal);
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
                var hVal:haxe.Int64 = cast h.value;
                all.set(hVal, c);
                var idx = -1;
                for (i in 0...handles.length) {
                    if (handles[i] == hVal) {
                        idx = i;
                        break;
                    }
                }
                if (idx == -1) {
                    handles.push(hVal);
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
        var res:Int = untyped __cpp__("fd_poll((size_t*)(void*){0}, (int*)(void*){1}, (int*)(void*){2}, {3}, {4})", nativeHandles, nativeEvents, nativeRevents, handles.length, timeoutMs);

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
