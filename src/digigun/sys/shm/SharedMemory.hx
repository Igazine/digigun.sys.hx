package digigun.sys.shm;

import haxe.io.Bytes;
import haxe.io.BytesData;
import digigun.sys.NativeHandle;

/**
 * Class for creating and accessing shared memory segments.
 */
class SharedMemory {
    private var handle:NativeHandle;
    private var name:String;

    public function new() {
        this.handle = NativeHandle.nullHandle();
    }

    /**
     * Creates or opens a shared memory segment.
     */
    public function open(name:String, size:Int, writable:Bool = true):Bool {
        #if cpp
        this.name = name;
        this.handle = new NativeHandle(Native.open_segment(name, size, writable ? 1 : 0));
        return this.handle.isValid;
        #else
        return false;
        #end
    }

    /**
     * Reads data from the shared memory segment.
     */
    public function read(offset:Int, length:Int):Bytes {
        #if cpp
        if (!this.handle.isValid) return null;
        var bytes = Bytes.alloc(length);
        var data:BytesData = bytes.getData();
        var ptr:cpp.RawPointer<cpp.Char> = cast cpp.NativeArray.address(data, 0);
        var res = Native.read_segment(this.handle.value, offset, ptr, length);
        return (res > 0) ? bytes.sub(0, res) : null;
        #else
        return null;
        #end
    }

    /**
     * Writes data to the shared memory segment.
     */
    public function write(offset:Int, bytes:Bytes):Int {
        #if cpp
        if (!this.handle.isValid) return -1;
        var data:BytesData = bytes.getData();
        var ptr:cpp.RawConstPointer<cpp.Char> = cast cpp.NativeArray.address(data, 0);
        return Native.write_segment(this.handle.value, offset, ptr, bytes.length);
        #else
        return -1;
        #end
    }

    /**
     * Closes the current process's connection to the shared memory.
     */
    public function close():Void {
        #if cpp
        if (this.handle.isValid) {
            Native.close_segment(this.handle.value);
            this.handle = NativeHandle.nullHandle();
        }
        #end
    }

    /**
     * Unlinks (deletes) the shared memory segment from the system.
     */
    public function unlink():Void {
        #if cpp
        if (this.name != null) Native.unlink_segment(this.name);
        #end
    }
}
