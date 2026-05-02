package digigun.sys.proc;

@:noDoc
#if cpp
@:include("proc_native.h")
extern class Native {
    private static function __init__():Void { digigun.sys.NativeBuild.init(); }

    @:native("proc_set_affinity")
    static function set_affinity(mask:Int):Int;

    @:native("proc_get_affinity")
    static function get_affinity():Int;

    @:native("proc_set_priority")
    static function set_priority(priorityClass:Int):Int;
}
#end
