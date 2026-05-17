package digigun.sys.fs;

import haxe.io.Bytes;

@:keep
@:include("fs_native.h")
private extern class Native {
    @:native("fs_symlink_create")
    static function create(target:cpp.ConstCharStar, linkpath:cpp.ConstCharStar):Int;

    @:native("fs_symlink_read")
    static function read(path:cpp.ConstCharStar, buffer:cpp.RawPointer<cpp.Char>, length:Int):Int;
}

/**
 * Platform-agnostic symbolic link management.
 */
class Symlink {
    /**
     * Creates a symbolic link at `linkPath` pointing to `targetPath`.
     * @param targetPath The existing file or directory the link will point to.
     * @param linkPath The path where the symlink will be created.
     * @return True if successful, false otherwise.
     */
    public static function create(targetPath:String, linkPath:String):Bool {
        return Native.create(targetPath, linkPath) == 0;
    }

    /**
     * Reads the target path of a symbolic link.
     * @param linkPath The path to the symbolic link.
     * @return The target path as a string, or null if it fails or is not a symlink.
     */
    public static function read(linkPath:String):String {
        var buffer = Bytes.alloc(1024);
        var ptr:cpp.RawPointer<cpp.Char> = cast untyped __cpp__("(char*)&{0}->b[0]", buffer);
        var len = Native.read(linkPath, ptr, buffer.length);
        if (len < 0) return null;
        return buffer.getString(0, len);
    }
}
