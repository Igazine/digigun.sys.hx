package digigun.sys.dl;

/**
 * Internal class to force inclusion of FFI symbols in Windows DLLs.
 */
@:keep
@:noDoc
@:cppFileCode('
extern "C" const char* process_echo(const char*);
extern "C" void process_point(void*);
extern "C" void process_complex(void*);
extern "C" long long loop_create();
extern "C" void loop_close(long long);
extern "C" int loop_submit(long long, long long, int, void*, int, void*, void*);
extern "C" int loop_poll(long long, int);
')
class FFIInternal {
    #if cpp
    @:native("process_echo")
    static function _process_echo(input:cpp.ConstCharStar):cpp.ConstCharStar return null;

    @:native("process_point")
    static function _process_point(p:cpp.RawPointer<cpp.Void>):Void {}

    @:native("process_complex")
    static function _process_complex(d:cpp.RawPointer<cpp.Void>):Void {}

    @:native("loop_create")
    static function _loop_create():haxe.Int64 return 0;

    @:native("loop_close")
    static function _loop_close(h:haxe.Int64):Void {}

    @:native("loop_submit")
    static function _loop_submit(h:haxe.Int64, f:haxe.Int64, o:Int, b:cpp.RawPointer<cpp.Void>, l:Int, c:cpp.RawPointer<cpp.Void>, u:cpp.RawPointer<cpp.Void>):Int return 0;

    @:native("loop_poll")
    static function _loop_poll(h:haxe.Int64, t:Int):Int return 0;

    public static function force():Void {
        if (false) {
            _process_echo(null);
            _process_point(null);
            _process_complex(null);
            _loop_create();
            _loop_close(0);
            _loop_submit(0, 0, 0, null, 0, null, null);
            _loop_poll(0, 0);
        }
    }
    #end
}
