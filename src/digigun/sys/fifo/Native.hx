package digigun.sys.fifo;

import digigun.sys.NativeHandle;

#if cpp
@:include("fifo_native.h")
extern class Native {
    private static function __init__():Void {
        digigun.sys.NativeBuild.init();
    }

    @:native("fifo_create")
    static function fifo_create(path:String, mode:Int):Int;

    @:native("fifo_open")
    static function fifo_open(path:String, writeMode:Int):cpp.SizeT;

    @:native("socket_create")
    static function socket_create():cpp.SizeT;

    @:native("socket_bind")
    static function socket_bind(handle:cpp.SizeT, path:String):Int;

    @:native("socket_listen")
    static function socket_listen(handle:cpp.SizeT, backlog:Int):Int;

    @:native("socket_accept")
    static function socket_accept(handle:cpp.SizeT):cpp.SizeT;

    @:native("socket_connect")
    static function socket_connect(handle:cpp.SizeT, path:String):Int;

    @:native("fd_read")
    static function fd_read(handle:cpp.SizeT, buffer:cpp.RawPointer<cpp.Char>, length:Int):Int;

    @:native("fd_write")
    static function fd_write(handle:cpp.SizeT, buffer:cpp.RawConstPointer<cpp.Char>, length:Int):Int;

    @:native("fd_set_blocking")
    static function fd_set_blocking(handle:cpp.SizeT, blocking:Int):Int;

    @:native("fd_poll")
    static function fd_poll(handles:cpp.RawPointer<cpp.SizeT>, events:cpp.RawPointer<Int>, revents:cpp.RawPointer<Int>, count:Int, timeout:Int):Int;

    @:native("fd_close")
    static function fd_close(handle:cpp.SizeT):Void;

    @:native("fd_get_numeric")
    static function fd_get_numeric(handle:cpp.SizeT):Int;
}
#end
