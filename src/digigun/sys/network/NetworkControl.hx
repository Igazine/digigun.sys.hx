package digigun.sys.network;

import cpp.Pointer;

typedef ArpEntry = {
    var ip:String;
    var mac:String;
    var interfaceName:String;
}

/**
 * Class for advanced network control and diagnostics.
 */
class NetworkControl {
    /**
     * Attempts to retrieve the system ARP table.
     * @return Array of ArpEntry objects.
     */
    public static function getArpTable():Array<ArpEntry> {
        #if cpp
        var headRaw = Native.get_arp_table();
        if (headRaw == null) return [];

        var result:Array<ArpEntry> = [];
        var current = Pointer.fromRaw(headRaw);

        while (current != null && current.raw != null) {
            var entry = current.value;
            result.push({
                ip: entry.ip.toString(),
                mac: entry.mac.toString(),
                interfaceName: entry.interface_name.toString()
            });
            current = Pointer.fromRaw(entry.next);
        }

        Native.free_arp_table(headRaw);
        return result;
        #else
        return [];
        #end
    }

    /**
     * Binds a standard Haxe Socket to a specific network interface by name.
     * 
     * Note: On Windows, this is currently a no-op/returns false as it requires 
     * different logic (binding to specific IP).
     * 
     * @param socket The Haxe sys.net.Socket to bind.
     * @param interfaceName The OS name of the interface (e.g. "en0", "eth0").
     * @return True if successful.
     */
    public static function bindToInterface(socket:sys.net.Socket, interfaceName:String):Bool {
        #if cpp
        var fd = getSocketFd(socket);
        if (fd == -1) return false;
        return Native.bind_to_interface(fd, interfaceName) == 0;
        #else
        return false;
        #end
    }

    /**
     * Sets an integer socket option.
     * @param socket The Haxe sys.net.Socket.
     * @param level The protocol level (e.g., SocketOption.SOL_SOCKET).
     * @param option The option name (e.g., SocketOption.SO_REUSEADDR).
     * @param value The integer value.
     * @return True if successful.
     */
    public static function setSocketOptionInt(socket:sys.net.Socket, level:Int, option:Int, value:Int):Bool {
        #if cpp
        var fd = getSocketFd(socket);
        if (fd == -1 || level == -1 || option == -1) return false;
        
        var val = cpp.Pointer.addressOf(value);
        return Native.set_socket_opt(fd, level, option, cast val.raw, 4) == 0;
        #else
        return false;
        #end
    }

    /**
     * Sets a boolean socket option.
     * @param socket The Haxe sys.net.Socket.
     * @param level The protocol level.
     * @param option The option name.
     * @param value The boolean value.
     * @return True if successful.
     */
    public static function setSocketOptionBool(socket:sys.net.Socket, level:Int, option:Int, value:Bool):Bool {
        return setSocketOptionInt(socket, level, option, value ? 1 : 0);
    }

    #if cpp
    private static function getSocketFd(socket:sys.net.Socket):Int {
        // Access internal socket handle in HXCPP
        var handle:Dynamic = untyped socket.__s;
        if (handle == null) return -1;
        
        // Use hxcpp internal conversion to get the raw socket descriptor
        return untyped __cpp__("(int)(intptr_t)({0}.mPtr ? {0}->__ToInt() : -1)", handle);
    }
    #end
}
