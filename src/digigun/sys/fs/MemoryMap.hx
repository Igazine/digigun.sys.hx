package digigun.sys.fs;

import haxe.io.Bytes;
import haxe.io.BytesData;
import digigun.sys.NativeHandle;
@:keep
@:include("fs_native.h")
private extern class Native {
    @:native("fs_mmap_open")
    static function mmap_open(path:cpp.ConstCharStar, size:Int, writable:Int):haxe.Int64;

    @:native("fs_mmap_close")
    static function mmap_close(id:haxe.Int64):Void;

    @:native("fs_mmap_read")
    static function mmap_read(id:haxe.Int64, offset:Int, buffer:cpp.RawPointer<cpp.Char>, length:Int):Int;

    @:native("fs_mmap_write")
    static function mmap_write(id:haxe.Int64, offset:Int, buffer:cpp.RawConstPointer<cpp.Char>, length:Int):Int;

    @:native("fs_mmap_flush")
    static function mmap_flush(id:haxe.Int64):Void;

    @:native("fs_mmap_get_address")
    static function mmap_get_address(id:haxe.Int64):haxe.Int64;
}

/**
 * Provides memory mapping of files for high-speed I/O.
 */
class MemoryMap {
    private var handle:NativeHandle;
    private var size:Int;

    private function new(h:NativeHandle, size:Int) {
        this.handle = h;
        this.size = size;
    }

    /**
     * Opens a file for memory mapping.
     * @param path Path to the file.
     * @param size Size of the mapping in bytes.
     * @param writable True to map with write access.
     * @return A MemoryMap instance if successful, null otherwise.
     */
    public static function open(path:String, size:Int, writable:Bool = false):MemoryMap {
        #if cpp
        var res = Native.mmap_open(path, size, writable ? 1 : 0);
        if (res != 0) {
            return new MemoryMap(new NativeHandle(res), size);
        }
        #end
        return null;
    }

    /**
     * Returns the raw memory address of the mapping.
     */
    public var address(get, never):haxe.Int64;
    private function get_address():haxe.Int64 {
        #if cpp
        return this.handle.isValid ? Native.mmap_get_address(this.handle.value) : 0;
        #else
        return 0;
        #end
    }

    /**
     * Returns a NativeBuffer view of the memory map.
     */
    public function asBuffer():digigun.sys.io.NativeBuffer {
        if (!this.handle.isValid) return null;
        return digigun.sys.io.NativeBuffer.fromAddress(this.address, this.size);
    }

    /**
     * Reads bytes from the memory map.
...
     * @param offset Start offset within the mapping.
     * @param length Number of bytes to read.
     * @return Bytes instance containing the data.
     */
    public function read(offset:Int, length:Int):Bytes {
        #if cpp
        if (!this.handle.isValid) return null;
        var buffer = Bytes.alloc(length);
        var data:BytesData = buffer.getData();
        var ptr:cpp.RawPointer<cpp.Char> = cast cpp.NativeArray.address(data, 0);
        var res = Native.mmap_read(this.handle.value, offset, ptr, length);
        return (res >= 0) ? buffer : null;
        #else
        return null;
        #end
    }

    /**
     * Writes bytes to the memory map.
     * @param offset Start offset within the mapping.
     * @param bytes Data to write.
     * @return True if successful.
     */
    public function write(offset:Int, bytes:Bytes):Bool {
        #if cpp
        if (!this.handle.isValid) return false;
        var data:BytesData = bytes.getData();
        var ptr:cpp.RawConstPointer<cpp.Char> = cast cpp.NativeArray.address(data, 0);
        return Native.mmap_write(this.handle.value, offset, ptr, bytes.length) >= 0;
        #else
        return false;
        #end
    }

    /**
     * Flushes changes to disk.
     */
    public function flush():Void {
        #if cpp
        if (this.handle.isValid) {
            Native.mmap_flush(this.handle.value);
        }
        #end
    }

    /**
     * Closes the memory mapping.
     */
    public function close():Void {
        #if cpp
        if (this.handle.isValid) {
            Native.mmap_close(this.handle.value);
            this.handle = NativeHandle.nullHandle();
        }
        #end
    }
}
