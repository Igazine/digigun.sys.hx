package digigun.sys.process;

import cpp.RawPointer;

@:noDoc
#if cpp
@:include("process_native.h")
@:native("NativeProcessInfo")
@:structAccess
extern class NativeProcessInfo {
    @:native("pid") var pid:Int;
    @:native("ppid") var ppid:Int;
    @:native("name") var name:cpp.ConstCharStar;
    @:native("memory_rss") var memory_rss:Float;
    @:native("memory_virtual") var memory_virtual:Float;
    @:native("next") var next:RawPointer<NativeProcessInfo>;
}

@:noDoc
@:include("process_native.h")
extern class Native {
    private static function __init__():Void { digigun.sys.NativeBuild.init(); }

    @:native("process_is_root")
    static function is_root():Int;

    @:native("process_get_file_limit")
    static function get_file_limit():Int;

    @:native("process_set_file_limit")
    static function set_file_limit(limit:Int):Int;

    @:native("process_list_all")
    static function list_all():RawPointer<NativeProcessInfo>;

    @:native("process_free_list")
    static function free_list(list:RawPointer<NativeProcessInfo>):Void;

    @:native("process_get_id")
    static function get_id():Int;

    @:native("process_fork")
    static function fork():Int;
}
#end
