package digigun.sys.fs;

import haxe.Int64;

/**
 * Platform-agnostic file types.
 */
enum abstract FileType(Int) from Int to Int {
    var BlockFile = 0;
    var CharacterFile = 1;
    var Directory = 2;
    var Empty = 3; // Assigned if size == 0 and type == Regular
    var Fifo = 4;
    var Other = 5;
    var RegularFile = 6;
    var ReparseFile = 7; // Windows specific (non-symlink)
    var Socket = 8;
    var Symlink = 9;

    public function toString():String {
        return switch (cast this : FileType) {
            case BlockFile: "BlockFile";
            case CharacterFile: "CharacterFile";
            case Directory: "Directory";
            case Empty: "Empty";
            case Fifo: "Fifo";
            case Other: "Other";
            case RegularFile: "RegularFile";
            case ReparseFile: "ReparseFile";
            case Socket: "Socket";
            case Symlink: "Symlink";
            default: "Unknown";
        }
    }
}

/**
 * Standard Unix file permission masks.
 */
enum abstract FilePermission(Int) from Int to Int {
    var OWNER_READ    = 0x0100; // 0400
    var OWNER_WRITE   = 0x0080; // 0200
    var OWNER_EXEC    = 0x0040; // 0100
    
    var GROUP_READ    = 0x0020; // 0040
    var GROUP_WRITE   = 0x0010; // 0020
    var GROUP_EXEC    = 0x0008; // 0010
    
    var OTHERS_READ   = 0x0004; // 0004
    var OTHERS_WRITE  = 0x0002; // 0002
    var OTHERS_EXEC   = 0x0001; // 0001

    var ALL_READ      = 0x0124;
    var ALL_WRITE     = 0x0092;
    var ALL_EXEC      = 0x0049;
}

/**
 * 64-bit file statistics.
 */
@:noDoc
@:structInit
class NativeFileStat {
    public var size:Int64;
    public var atime:Float;
    public var mtime:Float;
    public var ctime:Float;
    public var mode:Int;
    public var type:FileType;
}

@:noDoc
@:keep
@:include("fs_native.h")
private extern class Native {
    @:native("fs_stat")
    static function stat(path:cpp.ConstCharStar, out:cpp.RawPointer<cpp.Void>):Int;

    @:native("fs_chmod")
    static function chmod(path:cpp.ConstCharStar, mode:Int):Int;
}

/**
 * Advanced file system diagnostics and permission management.
 */
class FileDiagnostics {
    /**
     * Retrieves detailed 64-bit statistics for the given path.
     * @param path The filesystem path.
     * @return NativeFileStat or null if the path does not exist or an error occurred.
     */
    public static function stat(path:String):NativeFileStat {
        // We use a raw byte buffer to receive the C struct data
        // Struct layout in C: long long (8), double (8), double (8), double (8), int (4), int (4) = 40 bytes
        var bytes = haxe.io.Bytes.alloc(40);
        var ptr = cast untyped __cpp__("(void*)&{0}->b[0]", bytes);
        
        if (Native.stat(path, ptr) != 0) return null;
        
        // Extract values from the binary buffer
        var size:Int64 = untyped __cpp__("*(long long*)&{0}->b[0]", bytes);
        var atime:Float = untyped __cpp__("*(double*)&{0}->b[8]", bytes);
        var mtime:Float = untyped __cpp__("*(double*)&{0}->b[16]", bytes);
        var ctime:Float = untyped __cpp__("*(double*)&{0}->b[24]", bytes);
        var mode:Int = untyped __cpp__("*(int*)&{0}->b[32]", bytes);
        var type:Int = untyped __cpp__("*(int*)&{0}->b[36]", bytes);

        return {
            size: size,
            atime: atime,
            mtime: mtime,
            ctime: ctime,
            mode: mode,
            type: (type : FileType)
        };
    }

    /**
     * Changes the permissions (mode) of a file or directory.
     * On Windows, this is a best-effort mapping (Write bit toggles READONLY attribute).
     * @param path The filesystem path.
     * @param mode Bitmask of FilePermission values.
     * @return True if successful.
     */
    public static function chmod(path:String, mode:Int):Bool {
        return Native.chmod(path, mode) == 0;
    }

    /**
     * Convenience method to get just the file type.
     */
    public static function getType(path:String):FileType {
        var s = stat(path);
        return (s != null) ? s.type : Other;
    }
}
