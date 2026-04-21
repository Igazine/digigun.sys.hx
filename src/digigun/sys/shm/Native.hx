package digigun.sys.shm;

import digigun.sys.NativeHandle;

#if cpp
@:include("shm_native.h")
extern class Native {
    private static function __init__():Void { digigun.sys.NativeBuild.init(); }

    @:native("shm_open_segment")
    static function open_segment(name:String, size:Int, writable:Int):cpp.SizeT;

    @:native("shm_close_segment")
    static function close_segment(handle:cpp.SizeT):Void;

    @:native("shm_read_segment")
    static function read_segment(handle:cpp.SizeT, offset:Int, buffer:cpp.RawPointer<cpp.Char>, length:Int):Int;

    @:native("shm_write_segment")
    static function write_segment(handle:cpp.SizeT, offset:Int, buffer:cpp.RawConstPointer<cpp.Char>, length:Int):Int;

    @:native("shm_unlink_segment")
    static function unlink_segment(name:String):Void;
}
#end
