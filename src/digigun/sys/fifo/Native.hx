package digigun.sys.fifo;

import digigun.sys.NativeHandle;

@:noDoc
#if cpp
@:include("fifo_native.h")
extern class Native {
    private static function __init__():Void {
        digigun.sys.NativeBuild.init();
    }

    @:native("fifo_create")
    static function fifo_create(path:String, mode:Int):Int;

    @:native("fifo_open")
    static function fifo_open(path:String, writeMode:Int):Float;

    @:native("socket_create")
    static function socket_create():Float;

    @:native("socket_bind")
    static function socket_bind(handle:Float, path:String):Int;

    @:native("socket_listen")
    static function socket_listen(handle:Float, backlog:Int):Int;

    @:native("socket_accept")
    static function socket_accept(handle:Float):Float;

    @:native("socket_connect")
    static function socket_connect(handle:Float, path:String):Int;

    @:native("fd_read")
    static function fd_read(handle:Float, buffer:cpp.RawPointer<cpp.Char>, length:Int):Int;

    @:native("fd_write")
    static function fd_write(handle:Float, buffer:cpp.RawConstPointer<cpp.Char>, length:Int):Int;

    @:native("fd_set_blocking")
    static function fd_set_blocking(handle:Float, blocking:Int):Int;

    @:native("fd_poll")
    static function fd_poll(handles:cpp.RawPointer<haxe.Int64>, events:cpp.RawPointer<Int>, revents:cpp.RawPointer<Int>, count:Int, timeout:Int):Int;

    @:native("fd_close")
    static function fd_close(handle:Float):Void;

    @:native("fd_get_numeric")
    static function fd_get_numeric(handle:Float):Int;
}
#end
