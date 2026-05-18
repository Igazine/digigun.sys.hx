package digigun.sys.shm;

import haxe.io.Bytes;
import haxe.io.BytesData;
import digigun.sys.NativeHandle;
#if cpp
import cpp.Finalizable;
#end

@:keep
@:include("shm_native.h")
private extern class Native {
    @:native("shm_open_segment")
    static function open_segment(name:cpp.ConstCharStar, size:Int, writable:Int):haxe.Int64;

    @:native("shm_close_segment")
    static function close_segment(handle:haxe.Int64):Void;

    @:native("shm_read_segment")
    static function read_segment(handle:haxe.Int64, offset:Int, buffer:cpp.RawPointer<cpp.Char>, length:Int):Int;

    @:native("shm_write_segment")
    static function write_segment(handle:haxe.Int64, offset:Int, buffer:cpp.RawConstPointer<cpp.Char>, length:Int):Int;

    @:native("shm_unlink_segment")
    static function unlink_segment(name:cpp.ConstCharStar):Void;

    @:native("shm_get_address")
    static function get_address(handle:haxe.Int64):haxe.Int64;
}

/**
 * Class for creating and accessing shared memory segments.
 */
class SharedMemory #if cpp extends Finalizable #end {
    private var handle:NativeHandle;
    private var name:String;
    private var size:Int;

    public function new() {
        #if cpp
        super();
        #end
        this.handle = NativeHandle.nullHandle();
    }

    /**
     * Creates or opens a shared memory segment.
     */
    public function open(name:String, size:Int, writable:Bool = true):Bool {
        #if cpp
        this.name = name;
        this.size = size;
        this.handle = new NativeHandle(Native.open_segment(name, size, writable ? 1 : 0));
        return this.handle.isValid;
        #else
        return false;
        #end
    }

    /**
     * Returns the raw memory address of the shared segment.
     */
    public var address(get, never):haxe.Int64;
    private function get_address():haxe.Int64 {
        #if cpp
        return this.handle.isValid ? Native.get_address(this.handle.value) : 0;
        #else
        return 0;
        #end
    }

    /**
     * Returns a NativeBuffer view of the shared memory.
     * Note: The buffer is a view; closing SharedMemory invalidates the buffer.
     */
    public function asBuffer():digigun.sys.io.NativeBuffer {
        if (!this.handle.isValid) return null;
        return @:privateAccess digigun.sys.io.NativeBuffer._fromAddress(this.address, this.size);
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

    #if cpp
    override public function finalize():Void {
        close();
    }
    #end
}
