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
class MemoryMap #if cpp extends cpp.Finalizable #end {
    private var handle:NativeHandle;
    private var size:Int;
    private var _closed:Bool = false;

    private function new(h:NativeHandle, size:Int) {
        #if cpp
        super();
        #end
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
        return null;
        #else
        return null;
        #end
    }

    /**
     * Returns the raw memory address of the mapping.
     */
    public var address(get, never):haxe.Int64;
    private function get_address():haxe.Int64 {
        #if cpp
        return (this.handle.isValid && !_closed) ? Native.mmap_get_address(this.handle.value) : 0;
        #else
        return 0;
        #end
    }

    /**
     * Internal bridge to conversion to pointer.
     */
    @:noCompletion
    public function _getPointer():cpp.RawPointer<cpp.Void> {
        return (this.handle.isValid && !_closed) ? cast untyped __cpp__("(void*)(size_t){0}", this.address) : null;
    }

    /**
     * Reads data from the memory mapping.
     */
    public function read(offset:Int, length:Int):Bytes {
        #if cpp
        if (!this.handle.isValid || _closed) return null;
        var bytes = Bytes.alloc(length);
        var data:BytesData = bytes.getData();
        var ptr:cpp.RawPointer<cpp.Char> = cast cpp.NativeArray.address(data, 0);
        var res = Native.mmap_read(this.handle.value, offset, ptr, length);
        return (res > 0) ? bytes.sub(0, res) : null;
        #else
        return null;
        #end
        }

        /**
        * Writes data to the memory mapping.
        */
        public function write(offset:Int, bytes:Bytes):Int {
        #if cpp
        if (!this.handle.isValid || _closed) return -1;
        var data:BytesData = bytes.getData();
        var ptr:cpp.RawConstPointer<cpp.Char> = cast cpp.NativeArray.address(data, 0);
        return Native.mmap_write(this.handle.value, offset, ptr, bytes.length);
        #else
        return -1;
        #end
        }

    /**
     * Flushes changes to disk.
     */
    public function flush():Void {
        #if cpp
        if (this.handle.isValid && !_closed) Native.mmap_flush(this.handle.value);
        #end
    }

    /**
     * Closes the memory mapping.
     */
    public function close():Void {
        #if cpp
        if (this.handle.isValid && !_closed) {
            _closed = true;
            Native.mmap_close(this.handle.value);
        }
        #end
    }

    #if cpp
    override public function finalize():Void {
        close();
    }
    #end
}
