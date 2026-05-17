package digigun.sys.network;

import digigun.sys.NativeHandle;
import digigun.sys.io.NativeBuffer;

@:keep
@:include("network_native.h")
private extern class Native {
    @:native("network_raw_sniffer_open")
    static function open(interfaceName:cpp.ConstCharStar, promiscuous:Int):haxe.Int64;

    @:native("network_raw_sniffer_read")
    static function read(handle:haxe.Int64, buffer:cpp.RawPointer<cpp.Void>, length:Int):Int;

    @:native("network_raw_sniffer_close")
    static function close(handle:haxe.Int64):Void;
}

/**
 * High-performance raw network packet sniffer.
 * Requires Root (POSIX) or Administrator (Windows) privileges.
 */
class RawSocket {
    public var handle(default, null):NativeHandle;

    private function new(h:haxe.Int64) {
        this.handle = new NativeHandle(h);
    }

    /**
     * Creates a raw sniffer bound to an interface.
     * @param interfaceName Name of the interface (e.g. "en0", "eth0") or local IP on Windows.
     * @param promiscuous Whether to capture all traffic on the interface.
     */
    public static function create(interfaceName:String, promiscuous:Bool = true):RawSocket {
        var h = Native.open(interfaceName, promiscuous ? 1 : 0);
        if (h != 0) {
            return new RawSocket(h);
        }
        return null;
    }

    /**
     * Reads a raw packet into a native buffer.
     * @return Bytes read, or -1 on error.
     */
    public function readPacket(buffer:NativeBuffer):Int {
        if (!handle.isValid) return -1;
        return Native.read(handle.value, buffer._getPointer(), buffer.size);
    }

    /**
     * Closes the sniffer.
     */
    public function close():Void {
        if (handle.isValid) {
            Native.close(handle.value);
            handle = NativeHandle.nullHandle();
        }
    }
}
