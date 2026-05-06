package digigun.sys.fs;

import haxe.io.Bytes;
import haxe.io.BytesData;

/**
 * Provides access to Extended File Attributes (xattr on macOS/Linux, ADS on Windows).
 */
class ExtendedAttributes {
    /**
     * Sets an extended attribute on a file.
     */
    public static function set(path:String, name:String, value:Bytes):Bool {
        #if cpp
        var data:BytesData = value.getData();
        var ptr:cpp.RawPointer<cpp.UInt8> = cast cpp.NativeArray.address(data, 0);
        return Native.set_xattr(path, name, ptr, value.length) == 0;
        #else
        return false;
        #end
    }

    /**
     * Gets an extended attribute from a file.
     */
    public static function get(path:String, name:String):Bytes {
        #if cpp
        var size = Native.get_xattr(path, name, null, 0);
        if (size < 0) return null;
        var buffer = Bytes.alloc(size);
        var data:BytesData = buffer.getData();
        var ptr:cpp.RawPointer<cpp.UInt8> = cast cpp.NativeArray.address(data, 0);
        var res = Native.get_xattr(path, name, ptr, size);
        return (res >= 0) ? buffer : null;
        #else
        return null;
        #end
    }

    /**
     * Lists all extended attribute names for a file.
     */
    public static function list(path:String):Array<String> {
        #if cpp
        var size = Native.list_xattrs(path, null, 0);
        if (size <= 0) return [];
        var buffer = Bytes.alloc(size);
        var data:BytesData = buffer.getData();
        var ptr:cpp.RawPointer<cpp.Char> = cast cpp.NativeArray.address(data, 0);
        var res = Native.list_xattrs(path, ptr, size);
        if (res <= 0) return [];

        var result = [];
        var start = 0;
        for (i in 0...res) {
            if (data[i] == 0) {
                if (i > start) {
                    var bytes = buffer.sub(start, i - start);
                    result.push(bytes.toString());
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
     * Removes an extended attribute from a file.
     */
    public static function remove(path:String, name:String):Bool {
        #if cpp
        return Native.remove_xattr(path, name) == 0;
        #else
        return false;
        #end
    }
}
