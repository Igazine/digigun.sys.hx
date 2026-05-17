package digigun.sys.fs;

import haxe.Int64;

/**
 * Detailed volume and drive metadata.
 */
@:structInit
class VolumeInfo {
    public var totalSpace:Int64;
    public var freeSpace:Int64;
    public var fileSystem:String;
    public var name:String;
    public var uuid:String;
}

@:keep
@:include("info_native.h")
private extern class Native {
    @:native("info_get_volume_info")
    static function getVolumeInfo(path:cpp.ConstCharStar, out:cpp.RawPointer<cpp.Void>):Int;
}

/**
 * Provides detailed information about mounted filesystem volumes.
 */
class Volume {
    /**
     * Retrieves metadata for the volume containing the specified path.
     * @param path A path on the volume (e.g. "/" on POSIX, "C:\\" on Windows).
     * @return VolumeInfo instance or null if an error occurred.
     */
    public static function getInfo(path:String):VolumeInfo {
        // Struct layout: long long (8), long long (8), char[32] (32), char[128] (128), char[64] (64) = 240 bytes
        var bytes = haxe.io.Bytes.alloc(240);
        var ptr = cast untyped __cpp__("(void*)&{0}->b[0]", bytes);
        
        if (Native.getVolumeInfo(path, ptr) != 0) return null;
        
        var total:Int64 = untyped __cpp__("*(long long*)&{0}->b[0]", bytes);
        var free:Int64 = untyped __cpp__("*(long long*)&{0}->b[8]", bytes);
        
        // Use hidden internal _fromVoidPtr or similar logic for strings if needed, 
        // but here we just convert fixed-size char arrays.
        var fs:String = untyped __cpp__("String((const char*)&{0}->b[16])", bytes);
        var name:String = untyped __cpp__("String((const char*)&{0}->b[48])", bytes);
        var uuid:String = untyped __cpp__("String((const char*)&{0}->b[176])", bytes);

        return {
            totalSpace: total,
            freeSpace: free,
            fileSystem: fs,
            name: name,
            uuid: uuid
        };
    }
}
