package digigun.sys.fs;

import haxe.io.Bytes;
import haxe.io.BytesData;

/**
 * Class for high-performance memory-mapped file access.
 */
class MemoryMap {
    private var id:Float = -1.0;
    private var size:Int;

    /**
     * Maps a file into the process memory space.
     * @param path File path to map.
     * @param size Size of the mapping in bytes.
     * @param writable True for read/write access, false for read-only.
     * @return MemoryMap instance or null on failure.
     */
    public static function open(path:String, size:Int, writable:Bool = false):MemoryMap {
        #if cpp
        var res = Native.mmap_open(path, size, writable ? 1 : 0);
        if (res != -1.0) {
            var mmap = new MemoryMap();
            mmap.id = res;
            mmap.size = size;
            return mmap;
        }
        return null;
        #else
        return null;
        #end
    }

    /**
     * Closes the memory mapping.
     */
    public function close():Void {
        #if cpp
        if (id != -1.0) {
            Native.mmap_close(id);
            id = -1.0;
        }
        #end
    }

    /**
     * Reads bytes from the memory map.
     * @param offset Offset into the mapping.
     * @param length Number of bytes to read.
     * @return Bytes object containing the data, or null on error.
     */
    public function read(offset:Int, length:Int):Bytes {
        #if cpp
        if (id == -1.0) return null;
        var bytes = Bytes.alloc(length);
        var data:BytesData = bytes.getData();
        var ptr:cpp.RawPointer<cpp.Char> = cast cpp.NativeArray.address(data, 0);
        
        var read = Native.mmap_read(id, offset, ptr, length);
        if (read >= 0) return bytes;
        return null;
        #else
        return null;
        #end
    }

    /**
     * Writes bytes to the memory map.
     * @param offset Offset into the mapping.
     * @param value Bytes to write.
     * @return Number of bytes written, or -1 on error.
     */
    public function write(offset:Int, value:Bytes):Int {
        #if cpp
        if (id == -1.0) return -1;
        var data:BytesData = value.getData();
        var ptr:cpp.RawConstPointer<cpp.Char> = cast cpp.NativeArray.address(data, 0);
        
        return Native.mmap_write(id, offset, ptr, value.length);
        #else
        return -1;
        #end
    }

    /**
     * Flushes changes to the underlying file.
     */
    public function flush():Void {
        #if cpp
        if (id != -1.0) {
            Native.mmap_flush(id);
        }
        #end
    }

    private function new() {}
}
