package digigun.sys.random;

#if cpp
@:include("random_native.h")
extern class Native {
    private static function __init__():Void {
        digigun.sys.NativeBuild.init();
    }

    @:native("random_get_bytes")
    static function get_bytes(buffer:cpp.RawPointer<cpp.UInt8>, length:Int):Int;
}
#end
