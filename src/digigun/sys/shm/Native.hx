package digigun.sys.shm;

import digigun.sys.NativeHandle;

@:noDoc
#if cpp
@:include("shm_native.h")
extern class Native {
    private static function __init__():Void { digigun.sys.NativeBuild.init(); }

    @:native("shm_open_segment")
    static function open_segment(name:String, size:Int, writable:Int):Float;

    @:native("shm_close_segment")
    static function close_segment(handle:Float):Void;

    @:native("shm_read_segment")
    static function read_segment(handle:Float, offset:Int, buffer:cpp.RawPointer<cpp.Char>, length:Int):Int;

    @:native("shm_write_segment")
    static function write_segment(handle:Float, offset:Int, buffer:cpp.RawConstPointer<cpp.Char>, length:Int):Int;

    @:native("shm_unlink_segment")
    static function unlink_segment(name:String):Void;
}
#end
