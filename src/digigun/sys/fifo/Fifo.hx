package digigun.sys.fifo;

import cpp.RawPointer;
import haxe.io.Bytes;
import haxe.io.BytesData;
import digigun.sys.NativeHandle;

/**
 * Concrete implementation of a FIFO (Named Pipe) using native system calls.
 */
class Fifo implements IFifo {
    private var handle:NativeHandle;

    public function new() {
        this.handle = NativeHandle.nullHandle();
    }

    /**
     * Creates a new FIFO at the specified path.
     */
    public function create(path:String, mode:Int = 438):Bool {
        #if cpp
        return Native.fifo_create(path, mode) == 0;
        #else
        return false;
        #end
    }

    /**
     * Opens an existing FIFO.
     */
    public function open(path:String, writeMode:Bool):Bool {
        #if cpp
        this.handle = new NativeHandle(Native.fifo_open(path, writeMode ? 1 : 0));
        return this.handle.isValid;
        #else
        return false;
        #end
    }

    /**
     * Reads from the FIFO.
     */
    public function read(buffer:Bytes, length:Int):Int {
        #if cpp
        if (!this.handle.isValid) return -1;
        if (buffer.length < length) length = buffer.length;
        var data:BytesData = buffer.getData();
        var ptr:cpp.RawPointer<cpp.Char> = cast cpp.NativeArray.address(data, 0);
        var res = Native.fd_read(this.handle.value, ptr, length);
        if (res == -2) throw haxe.io.Error.Blocked;
        return res;
        #else
        return -1;
        #end
    }

    /**
     * Writes to the FIFO.
     */
    public function write(buffer:Bytes, length:Int):Int {
        #if cpp
        if (!this.handle.isValid) return -1;
        if (buffer.length < length) length = buffer.length;
        var data:BytesData = buffer.getData();
        var ptr:cpp.RawConstPointer<cpp.Char> = cast cpp.NativeArray.address(data, 0);
        var res = Native.fd_write(this.handle.value, ptr, length);
        if (res == -2) throw haxe.io.Error.Blocked;
        return res;
        #else
        return -1;
        #end
    }

    public function setBlocking(blocking:Bool):Bool {
        #if cpp
        if (!this.handle.isValid) return false;
        return Native.fd_set_blocking(this.handle.value, blocking ? 1 : 0) == 0;
        #else
        return false;
        #end
    }

    public function getHandle():NativeHandle {
        return this.handle;
    }

    public function close():Void {
        #if cpp
        if (this.handle.isValid) {
            Native.fd_close(this.handle.value);
            this.handle = NativeHandle.nullHandle();
        }
        #end
    }
}
