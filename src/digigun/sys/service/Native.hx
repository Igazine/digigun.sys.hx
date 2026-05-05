package digigun.sys.service;

import cpp.Callable;

#if cpp
@:include("service_native.h")
extern class Native {
    private static function __init__():Void { digigun.sys.NativeBuild.init(); }

    @:native("service_notify")
    static function notify(status:cpp.ConstCharStar):Int;

    @:native("service_is_available")
    static function is_available():Int;

    @:native("service_run")
    static function run(name:cpp.ConstCharStar, onStart:Callable<()->Void>, onStop:Callable<()->Void>):Int;
}
#end
