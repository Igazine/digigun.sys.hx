package digigun.sys.dl;

#if cpp
@:include("dl_native.h")
extern class Native {
    private static function __init__():Void { digigun.sys.NativeBuild.init(); }

    @:native("dl_open")
    static function dl_open(path:cpp.ConstCharStar):haxe.Int64;

    @:native("dl_get_symbol")
    static function dl_get_symbol(handle:haxe.Int64, name:cpp.ConstCharStar):haxe.Int64;

    @:native("dl_close")
    static function dl_close(handle:haxe.Int64):Void;

    @:native("dl_get_error")
    static function dl_get_error():cpp.ConstCharStar;
}
#end
