package digigun.sys.network;

/**
 * Common socket option levels and names.
 * These are resolved dynamically from the native layer to ensure cross-platform correctness.
 */
class SocketOption {
    // Levels
    public static var SOL_SOCKET(get, never):Int;
    public static var IPPROTO_TCP(get, never):Int;

    // Options
    public static var TCP_NODELAY(get, never):Int;
    public static var SO_REUSEADDR(get, never):Int;
    public static var SO_REUSEPORT(get, never):Int;
    public static var SO_KEEPALIVE(get, never):Int;
    public static var SO_RCVBUF(get, never):Int;
    public static var SO_SNDBUF(get, never):Int;

    private static var _cache = new Map<String, Int>();

    private static function get_SOL_SOCKET() return v("SOL_SOCKET");
    private static function get_IPPROTO_TCP() return v("IPPROTO_TCP");
    private static function get_TCP_NODELAY() return v("TCP_NODELAY");
    private static function get_SO_REUSEADDR() return v("SO_REUSEADDR");
    private static function get_SO_REUSEPORT() return v("SO_REUSEPORT");
    private static function get_SO_KEEPALIVE() return v("SO_KEEPALIVE");
    private static function get_SO_RCVBUF() return v("SO_RCVBUF");
    private static function get_SO_SNDBUF() return v("SO_SNDBUF");

    private static function v(name:String):Int {
        if (_cache.exists(name)) return _cache.get(name);
        #if cpp
        var val = Native.get_constant(name);
        _cache.set(name, val);
        return val;
        #else
        return -1;
        #end
    }
}
