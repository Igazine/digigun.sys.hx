package digigun.sys.io;

import digigun.sys.NativeHandle;

@:noDoc
#if cpp
@:include("io_native.h")
extern class Native {
    private static function __init__():Void { digigun.sys.NativeBuild.init(); }

    @:native("io_sendfile")
    static function sendfile(out_handle:Float, path:String, offset:haxe.Int64, length:haxe.Int64):haxe.Int64;

    @:native("io_set_direct_io")
    static function set_direct_io(handle:Float, enabled:Int):Int;

    @:native("io_open_file")
    static function open_file(path:String, write_mode:Int):Float;

    @:native("io_close_file")
    static function close_file(handle:Float):Void;
}
#end
