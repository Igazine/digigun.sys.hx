package digigun.sys.time;

import haxe.Int64;

#if cpp
@:include("time_native.h")
extern class Native {
    private static function __init__():Void { digigun.sys.NativeBuild.init(); }

    @:native("time_stamp")
    static function stamp():Float;

    @:native("time_nano_stamp")
    static function nano_stamp():Int64;
}
#end
