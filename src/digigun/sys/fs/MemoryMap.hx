package digigun.sys.fs;

import haxe.io.Bytes;
import haxe.io.BytesData;
import cpp.NativeArray;

/**
 * Class for high-performance memory-mapped file access.
 */
class MemoryMap {
    private var id:Int = -1;
    private var size:Int = 0;

    private function new(id:Int, size:Int) {
        this.id = id;
        this.size = size;
    }

    /**
     * Maps a file into memory.
     * @param path The path to the file.
     * @param size The size of the mapping (0 for current file size).
     * @param writable If true, allows writing to the mapped memory.
     * @return A MemoryMap instance, or null on error.
     */
    public static function open(path:String, size:Int = 0, writable:Bool = true):MemoryMap {
        #if cpp
        var id = Native.mmap_open(path, size, writable ? 1 : 0);
        if (id == -1) return null;
        
        // We need to know the actual mapped size if 0 was passed
        // For simplicity in this bridge, the native side should return the size
        // but for now let's assume we map what's requested or file size.
        return new MemoryMap(id, size);
        #else
        return null;
        #end
    }

    /**
     * Reads bytes from the mapped file.
     * @param offset Offset from start.
     * @param length Number of bytes.
     * @return Bytes object.
     */
    public function read(offset:Int, length:Int):Bytes {
        #if cpp
        if (this.id == -1) return null;
        var bytes = Bytes.alloc(length);
        var data:BytesData = bytes.getData();
        var ptr = cast NativeArray.address(data, 0);
        
        var readCount = Native.mmap_read(this.id, offset, ptr, length);
        if (readCount == -1) return null;
        if (readCount < length) return bytes.sub(0, readCount);
        return bytes;
        #else
        return null;
        #end
    }

    /**
     * Writes bytes to the mapped file.
     * @param offset Offset from start.
     * @param data The bytes to write.
     * @return Number of bytes written.
     */
    public function write(offset:Int, data:Bytes):Int {
        #if cpp
        if (this.id == -1) return -1;
        var bytesData:BytesData = data.getData();
        var ptr = cast NativeArray.address(bytesData, 0);
        return Native.mmap_write(this.id, offset, ptr, data.length);
        #else
        return -1;
        #end
    }

    /**
     * Flushes changes to disk.
     */
    public function flush():Void {
        #if cpp
        if (this.id != -1) Native.mmap_flush(this.id);
        #end
    }

    /**
     * Unmaps the memory and closes the file.
     */
    public function close():Void {
        #if cpp
        if (this.id != -1) {
            Native.mmap_close(this.id);
            this.id = -1;
        }
        #end
    }
}
