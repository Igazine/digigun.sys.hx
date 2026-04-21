package digigun.sys.network;

import digigun.sys.network.NetworkInterfaceInfo;
import cpp.Pointer;

/**
 * Class providing static network utility functions.
 */
class Network {
    /**
     * Resolves a hostname to a list of IP addresses.
     * @param host The hostname to resolve.
     * @return An array of IP address strings.
     */
    public static function getHostInfo(host:String):Array<String> {
        #if cpp
        var headRaw = Native.get_host_info(host);
        if (headRaw == null) return [];
        
        var result:Array<String> = [];
        var current = Pointer.fromRaw(headRaw);
        
        while (current != null && current.raw != null) {
            result.push(current.value.ip.toString());
            current = Pointer.fromRaw(current.value.next);
        }
        
        Native.free_host_info(headRaw);
        return result;
        #else
        return [];
        #end
    }

    /**
     * Retrieves a list of all available network interfaces and their details.
     * @return An array of NetworkInterfaceInfo objects.
     */
    public static function getNetworkInterfaces():Array<NetworkInterfaceInfo> {
        #if cpp
        var headRaw = Native.get_interfaces();
        if (headRaw == null) return [];
        
        var result:Array<NetworkInterfaceInfo> = [];
        var current = Pointer.fromRaw(headRaw);
        
        while (current != null && current.raw != null) {
            var ni = current.value;
            result.push({
                name: ni.name.toString(),
                ipv4: ni.ipv4.toString(),
                ipv6: ni.ipv6.toString(),
                mac: ni.mac.toString(),
                flags: ni.flags
            });
            current = Pointer.fromRaw(ni.next);
        }
        
        Native.free_interfaces(headRaw);
        return result;
        #else
        return [];
        #end
    }

    /**
     * Sends an ICMP Echo Request to the specified host and waits for a reply.
     * @param host The hostname or IP address to ping.
     * @param timeoutMs Timeout in milliseconds (default 1000).
     * @return The round-trip time in milliseconds, or -1.0 on error/timeout.
     */
    public static function ping(host:String, timeoutMs:Int = 1000):Float {
        #if cpp
        return Native.ping(host, timeoutMs);
        #else
        return -1.0;
        #end
    }
}
