package digigun.sys.service;

import cpp.Callable;

/**
 * Provides integration with system service managers (systemd, SCM).
 */
class Service {
    /**
     * Common systemd/SCM notification strings.
     */
    public static inline var READY = "READY=1";
    public static inline var RELOADING = "RELOADING=1";
    public static inline var STOPPING = "STOPPING=1";

    /**
     * Reports status to the system service manager.
     * 
     * On Linux (systemd), this implements the sd_notify protocol.
     * On Windows (SCM), this updates the service status.
     * 
     * @param status The status string to send (e.g., Service.READY or "STATUS=Working").
     * @return True if notification was successfully sent.
     */
    public static function notify(status:String):Bool {
        #if cpp
        return Native.notify(status) == 0;
        #else
        return false;
        #end
    }

    /**
     * Checks if the application is running under a system service manager.
     * 
     * @return True if running as a service and notifications are possible.
     */
    public static function isAvailable():Bool {
        #if cpp
        return Native.is_available() == 1;
        #else
        return false;
        #end
    }

    /**
     * Runs the application as a system service.
     * 
     * On Windows, this performs the SCM handshake and blocks until the service is stopped.
     * On POSIX, this executes onStart() and returns immediately (it does not daemonize).
     * 
     * @param name The name of the service (Windows only).
     * @param onStart Callback when the service starts.
     * @param onStop Callback when the service is requested to stop.
     * @return 0 on success, or OS error code on failure.
     */
    public static function run(name:String, onStart:()->Void, onStop:()->Void):Int {
        #if cpp
        // We use static wrappers to pass to C++ Callable
        _onStart = onStart;
        _onStop = onStop;
        return Native.run(name, Callable.fromStaticFunction(dispatchStart), Callable.fromStaticFunction(dispatchStop));
        #else
        onStart();
        return 0;
        #end
    }

    #if cpp
    private static var _onStart:()->Void;
    private static var _onStop:()->Void;

    private static function dispatchStart():Void {
        if (_onStart != null) _onStart();
    }

    private static function dispatchStop():Void {
        if (_onStop != null) _onStop();
    }
    #end
}
