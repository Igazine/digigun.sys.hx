package digigun.sys.time;

import haxe.Int64;

@:noDoc
#if cpp
@:include("time_native.h")
extern class Native {
    private static function __init__():Void { digigun.sys.NativeBuild.init(); }

    @:native("time_stamp")
    static function stamp():Float;

    @:native("time_nano_stamp")
    static function nano_stamp():Int64;

    @:native("time_sleep_nanos")
    static function sleep_nanos(nanos:Int64):Void;
}
#end
