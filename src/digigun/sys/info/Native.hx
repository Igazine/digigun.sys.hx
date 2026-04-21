package digigun.sys.info;

import cpp.Pointer;

#if cpp
@:include("info_native.h")
extern class Native {
    private static function __init__():Void { digigun.sys.NativeBuild.init(); }

    @:native("info_get_memory")
    static function get_memory(total:Pointer<Float>, free:Pointer<Float>, used:Pointer<Float>):Void;

    @:native("info_get_cpu_usage")
    static function get_cpu_usage():Float;

    @:native("info_get_disk")
    static function get_disk(path:String, total:Pointer<Float>, free:Pointer<Float>, avail:Pointer<Float>):Void;
}
#end
