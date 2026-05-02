package digigun.sys.rt;

@:noDoc
#if cpp
@:include("rt_native.h")
extern class Native {
    private static function __init__():Void { digigun.sys.NativeBuild.init(); }

    @:native("rt_mlockall")
    static function mlockall(current:Int, future:Int):Int;

    @:native("rt_munlockall")
    static function munlockall():Int;
}
#end
