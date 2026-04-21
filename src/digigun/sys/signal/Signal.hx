package digigun.sys.signal;

/**
 * High-level API for native signal handling.
 * Signal values are retrieved dynamically from the system to ensure cross-platform accuracy.
 */
class Signal {
    public static var INT(get, never):Int;
    public static var ILL(get, never):Int;
    public static var FPE(get, never):Int;
    public static var SEGV(get, never):Int;
    public static var TERM(get, never):Int;
    public static var BREAK(get, never):Int;
    public static var ABRT(get, never):Int;
    
    // POSIX specific
    public static var HUP(get, never):Int;
    public static var QUIT(get, never):Int;
    public static var TRAP(get, never):Int;
    public static var KILL(get, never):Int;
    public static var BUS(get, never):Int;
    public static var USR1(get, never):Int;
    public static var USR2(get, never):Int;

    private static var _cache = new Map<String, Int>();

    private static function get_INT() return v("INT");
    private static function get_ILL() return v("ILL");
    private static function get_FPE() return v("FPE");
    private static function get_SEGV() return v("SEGV");
    private static function get_TERM() return v("TERM");
    private static function get_BREAK() return v("BREAK");
    private static function get_ABRT() return v("ABRT");
    private static function get_HUP() return v("HUP");
    private static function get_QUIT() return v("QUIT");
    private static function get_TRAP() return v("TRAP");
    private static function get_KILL() return v("KILL");
    private static function get_BUS() return v("BUS");
    private static function get_USR1() return v("USR1");
    private static function get_USR2() return v("USR2");

    private static function v(name:String):Int {
        if (_cache.exists(name)) return _cache.get(name);
        #if cpp
        var val = Native.get_value(name);
        _cache.set(name, val);
        return val;
        #else
        return -1;
        #end
    }

    /**
     * Traps a native signal and executes the provided callback.
     * @param signo The signal number to trap.
     * @param callback Function to call when the signal is received.
     * @return True if successful.
     */
    public static function trap(signo:Int, callback:(signo:Int)->Void):Bool {
        if (signo == -1) return false;
        #if cpp
        return untyped __cpp__("signal_trap({0}, {1}.mPtr)", signo, callback) == 0;
        #else
        return false;
        #end
    }

    /**
     * Sends a signal to the current process.
     * @param signo The signal number to raise.
     * @return True if successful.
     */
    public static function raise(signo:Int):Bool {
        if (signo == -1) return false;
        #if cpp
        return Native.raise(signo) == 0;
        #else
        return false;
        #end
    }
}
