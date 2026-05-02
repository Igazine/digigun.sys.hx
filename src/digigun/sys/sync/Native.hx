package digigun.sys.sync;

import digigun.sys.NativeHandle;

@:noDoc
#if cpp
@:include("sync_native.h")
extern class Native {
    private static function __init__():Void { digigun.sys.NativeBuild.init(); }

    @:native("sync_sem_open")
    static function sem_open(name:String, initial_value:Int):Float;

    @:native("sync_sem_wait")
    static function sem_wait(handle:Float):Int;

    @:native("sync_sem_post")
    static function sem_post(handle:Float):Int;

    @:native("sync_sem_trywait")
    static function sem_trywait(handle:Float):Int;

    @:native("sync_sem_close")
    static function sem_close(handle:Float):Void;

    @:native("sync_sem_unlink")
    static function sem_unlink(name:String):Void;
}
#end
