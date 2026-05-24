package digigun.sys.sync;

import digigun.sys.NativeHandle;

@:noDoc
#if cpp
@:include("sync_native.h")
extern class Native {
    private static function __init__():Void { digigun.sys.NativeBuild.init(); }

    @:native("sync_sem_open")
    static function sem_open(name:String, initial_value:Int):haxe.Int64;

    @:native("sync_sem_wait")
    static function sem_wait(handle:haxe.Int64):Int;

    @:native("sync_sem_post")
    static function sem_post(handle:haxe.Int64):Int;

    @:native("sync_sem_trywait")
    static function sem_trywait(handle:haxe.Int64):Int;

    @:native("sync_sem_close")
    static function sem_close(handle:haxe.Int64):Void;

    @:native("sync_sem_unlink")
    static function sem_unlink(name:String):Void;

    @:native("sync_futex_wait")
    static function futex_wait(addr:cpp.RawPointer<Int>, expectedValue:Int):Int;

    @:native("sync_futex_wake")
    static function futex_wake(addr:cpp.RawPointer<Int>):Int;

    @:native("sync_futex_wake_all")
    static function futex_wake_all(addr:cpp.RawPointer<Int>):Int;
}
#end
