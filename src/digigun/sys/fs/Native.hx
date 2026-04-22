package digigun.sys.fs;

import cpp.Callable;

#if cpp
@:include("fs_native.h")
extern class Native {
    private static function __init__():Void { digigun.sys.NativeBuild.init(); }

    @:native("fs_watch_start")
    static function watch_start(path:String, callback:Callable<(cpp.ConstCharStar, Int, Int)->Void>):Int;

    @:native("fs_watch_stop_all")
    static function watch_stop_all():Void;

    @:native("fs_file_lock")
    static function file_lock(path:String, exclusive:Int, wait:Int):Int;

    @:native("fs_file_unlock")
    static function file_unlock(id:Int):Void;

    @:native("fs_mmap_open")
    static function mmap_open(path:String, size:Int, writable:Int):Int;

    @:native("fs_mmap_close")
    static function mmap_close(id:Int):Void;

    @:native("fs_mmap_read")
    static function mmap_read(id:Int, offset:Int, buffer:cpp.RawPointer<cpp.Char>, length:Int):Int;

    @:native("fs_mmap_write")
    static function mmap_write(id:Int, offset:Int, buffer:cpp.RawConstPointer<cpp.Char>, length:Int):Int;

    @:native("fs_mmap_flush")
    static function mmap_flush(id:Int):Void;

    @:native("fs_set_xattr")
    static function set_xattr(path:String, name:String, value:cpp.RawConstPointer<cpp.UInt8>, length:Int):Int;

    @:native("fs_get_xattr")
    static function get_xattr(path:String, name:String, buffer:cpp.RawPointer<cpp.UInt8>, length:Int):Int;

    @:native("fs_list_xattrs")
    static function list_xattrs(path:String, buffer:cpp.RawPointer<cpp.Char>, length:Int):Int;

    @:native("fs_remove_xattr")
    static function remove_xattr(path:String, name:String):Int;
}
#end
