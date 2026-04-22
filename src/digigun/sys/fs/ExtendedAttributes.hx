package digigun.sys.fs;

import haxe.io.Bytes;
import haxe.io.BytesData;

/**
 * Provides access to extended file attributes (xattr on POSIX, Alternate Data Streams on Windows).
 */
class ExtendedAttributes {
    /**
     * Sets an extended attribute on a file or directory.
     * @param path Path to the file or directory.
     * @param name Name of the attribute.
     * @param value Value of the attribute as Bytes.
     * @return True on success, false otherwise.
     */
    public static function set(path:String, name:String, value:Bytes):Bool {
        #if cpp
        var data:BytesData = value.getData();
        var ptr:cpp.RawConstPointer<cpp.UInt8> = cast cpp.NativeArray.address(data, 0);
        return Native.set_xattr(path, name, ptr, value.length) == 0;
        #else
        return false;
        #end
    }

    /**
     * Retrieves the value of an extended attribute.
     * @param path Path to the file or directory.
     * @param name Name of the attribute.
     * @return The attribute value as Bytes, or null if not found or on error.
     */
    public static function get(path:String, name:String):Bytes {
        #if cpp
        // First, get the required buffer size
        var size = Native.get_xattr(path, name, null, 0);
        if (size < 0) return null;
        if (size == 0) return Bytes.alloc(0);

        var bytes = Bytes.alloc(size);
        var data:BytesData = bytes.getData();
        var ptr:cpp.RawPointer<cpp.UInt8> = cast cpp.NativeArray.address(data, 0);
        
        var read = Native.get_xattr(path, name, ptr, size);
        if (read >= 0) return bytes;
        return null;
        #else
        return null;
        #end
    }

    /**
     * Lists all extended attribute names on a file or directory.
     * @param path Path to the file or directory.
     * @return Array of attribute names.
     */
    public static function list(path:String):Array<String> {
        #if cpp
        // First, get the required buffer size
        var size = Native.list_xattrs(path, null, 0);
        if (size <= 0) return [];

        var bytes = Bytes.alloc(size);
        var data:BytesData = bytes.getData();
        var ptr:cpp.RawPointer<cpp.Char> = cast cpp.NativeArray.address(data, 0);

        var read = Native.list_xattrs(path, ptr, size);
        if (read <= 0) return [];

        var result = [];
        var start = 0;
        for (i in 0...read) {
            if (bytes.get(i) == 0) {
                if (i > start) {
                    result.push(bytes.getString(start, i - start));
                }
                start = i + 1;
            }
        }
        return result;
        #else
        return [];
        #end
    }

    /**
     * Removes an extended attribute from a file or directory.
     * @param path Path to the file or directory.
     * @param name Name of the attribute.
     * @return True on success, false otherwise.
     */
    public static function remove(path:String, name:String):Bool {
        #if cpp
        return Native.remove_xattr(path, name) == 0;
        #else
        return false;
        #end
    }
}
