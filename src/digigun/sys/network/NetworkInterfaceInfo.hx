package digigun.sys.network;

/**
 * Structure representing network interface information.
 */
typedef NetworkInterfaceInfo = {
    /**
     * Interface name (e.g., "en0", "eth0").
     */
    var name:String;

    /**
     * IPv4 address assigned to the interface.
     */
    var ipv4:String;

    /**
     * IPv6 address assigned to the interface.
     */
    var ipv6:String;

    /**
     * MAC address (Hardware address).
     */
    var mac:String;

    /**
     * Interface flags representing configuration and status (bitmask).
     * Common bitmask values (POSIX):
     * - 0x1: IFF_UP (Interface is administratively up)
     * - 0x2: IFF_BROADCAST (Broadcast address valid)
     * - 0x8: IFF_LOOPBACK (Is a loopback net)
     * - 0x10: IFF_POINTOPOINT (Is a point-to-point link)
     * - 0x40: IFF_RUNNING (Resources allocated / link is up)
     * - 0x8000: IFF_MULTICAST (Supports multicast)
     */
    var flags:Int;
}
