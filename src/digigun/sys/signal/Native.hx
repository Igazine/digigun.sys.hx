package digigun.sys.signal;

#if cpp
@:include("signal_native.h")
extern class Native {
    private static function __init__():Void { digigun.sys.NativeBuild.init(); }

    @:native("signal_raise")
    static function raise(signo:Int):Int;

    @:native("signal_get_value")
    static function get_value(name:String):Int;
}
#end
