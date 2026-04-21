package digigun.sys.network;

import cpp.RawPointer;

#if cpp
@:include("network_native.h")
@:native("NativeIP")
@:structAccess
extern class NativeIP {
    @:native("ip") var ip:cpp.ConstCharStar;
    @:native("next") var next:RawPointer<NativeIP>;
}

@:include("network_native.h")
@:native("NativeInterface")
@:structAccess
extern class NativeInterface {
    @:native("name") var name:cpp.ConstCharStar;
    @:native("ipv4") var ipv4:cpp.ConstCharStar;
    @:native("ipv6") var ipv6:cpp.ConstCharStar;
    @:native("mac") var mac:cpp.ConstCharStar;
    @:native("flags") var flags:Int;
    @:native("next") var next:RawPointer<NativeInterface>;
}

@:include("network_native.h")
@:native("NativeArpEntry")
@:structAccess
extern class NativeArpEntry {
    @:native("ip") var ip:cpp.ConstCharStar;
    @:native("mac") var mac:cpp.ConstCharStar;
    @:native("interface_name") var interface_name:cpp.ConstCharStar;
    @:native("next") var next:RawPointer<NativeArpEntry>;
}

@:include("network_native.h")
extern class Native {
    private static function __init__():Void { digigun.sys.NativeBuild.init(); }

    @:native("network_get_host_info")
    static function get_host_info(host:String):RawPointer<NativeIP>;

    @:native("network_free_host_info")
    static function free_host_info(list:RawPointer<NativeIP>):Void;

    @:native("network_get_interfaces")
    static function get_interfaces():RawPointer<NativeInterface>;

    @:native("network_free_interfaces")
    static function free_interfaces(list:RawPointer<NativeInterface>):Void;

    @:native("network_ping")
    static function ping(host:String, timeoutMs:Int):Float;

    @:native("network_get_arp_table")
    static function get_arp_table():RawPointer<NativeArpEntry>;

    @:native("network_free_arp_table")
    static function free_arp_table(list:RawPointer<NativeArpEntry>):Void;

    @:native("network_bind_to_interface")
    static function bind_to_interface(socket_fd:Int, interface_name:String):Int;
}
#end
