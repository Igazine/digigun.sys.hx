package digigun.sys.auth;

import cpp.RawPointer;

@:noDoc
#if cpp
@:include("auth_native.h")
@:native("NativeUserInfo")
@:structAccess
extern class NativeUserInfo {
    @:native("uid") var uid:Int;
    @:native("gid") var gid:Int;
    @:native("username") var username:cpp.ConstCharStar;
    @:native("realname") var realname:cpp.ConstCharStar;
    @:native("home_dir") var home_dir:cpp.ConstCharStar;
    @:native("shell") var shell:cpp.ConstCharStar;
    @:native("next") var next:RawPointer<NativeUserInfo>;
}

@:noDoc
@:include("auth_native.h")
@:native("NativeGroupInfo")
@:structAccess
extern class NativeGroupInfo {
    @:native("gid") var gid:Int;
    @:native("name") var name:cpp.ConstCharStar;
    @:native("next") var next:RawPointer<NativeGroupInfo>;
}

@:noDoc
@:include("auth_native.h")
extern class Native {
    private static function __init__():Void {
        digigun.sys.NativeBuild.init();
    }

    @:native("auth_get_user_by_uid")
    static function get_user_by_uid(uid:Int):RawPointer<NativeUserInfo>;

    @:native("auth_get_user_by_name")
    static function get_user_by_name(name:String):RawPointer<NativeUserInfo>;

    @:native("auth_get_current_user")
    static function get_current_user():RawPointer<NativeUserInfo>;

    @:native("auth_free_user")
    static function free_user(user:RawPointer<NativeUserInfo>):Void;

    @:native("auth_get_group_by_gid")
    static function get_group_by_gid(gid:Int):RawPointer<NativeGroupInfo>;

    @:native("auth_get_groups")
    static function get_groups():RawPointer<NativeGroupInfo>;

    @:native("auth_free_groups")
    static function free_groups(groups:RawPointer<NativeGroupInfo>):Void;
}
#end
