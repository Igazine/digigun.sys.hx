package digigun.sys.console;

import cpp.Pointer;

@:noDoc
#if cpp
@:include("console_native.h")
extern class Native {
    private static function __init__():Void { digigun.sys.NativeBuild.init(); }

    @:native("console_set_raw_mode")
    static function set_raw_mode(enabled:Int):Int;

    @:native("console_get_size")
    static function get_size(width:Pointer<Int>, height:Pointer<Int>):Void;

    @:native("console_write_block")
    static function write_block(x:Int, y:Int, width:Int, height:Int, chars:cpp.RawConstPointer<Int>, attrs:cpp.RawConstPointer<Int>):Void;

    @:native("console_read_block")
    static function read_block(x:Int, y:Int, width:Int, height:Int, chars:cpp.RawPointer<Int>, attrs:cpp.RawPointer<Int>):Void;

    @:native("console_set_cursor_visible")
    static function set_cursor_visible(visible:Int):Void;

    @:native("console_use_alternate_buffer")
    static function use_alternate_buffer(enable:Int):Void;
}
#end
