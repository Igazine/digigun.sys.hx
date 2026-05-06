package digigun.sys.fs;

import haxe.io.Bytes;
import haxe.io.BytesData;
import digigun.sys.NativeHandle;

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
     * Reads bytes from the memory map.
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
