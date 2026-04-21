package digigun.sys.fs;

import cpp.Callable;
import digigun.sys.fs.FSEvent;

/**
 * Class for high-performance, native file system watching.
 */
class Watcher {
    private static var _callback:(FSEvent)->Void = null;

    /**
     * Starts a recursive watch on the specified directory.
     * @param path Directory to watch.
     * @param callback Function to call when an event occurs.
     * @return True if watch started successfully.
     */
    public static function watch(path:String, callback:(FSEvent)->Void):Bool {
        #if cpp
        _callback = callback;
        var nativeCallback = Callable.fromStaticFunction(_onNativeEvent);
        return Native.watch_start(path, nativeCallback) == 1;
        #else
        return false;
        #end
    }

    /**
     * Stops all active native watches and background threads.
     */
    public static function stopAll():Void {
        #if cpp
        Native.watch_stop_all();
        _callback = null;
        #end
    }

    /**
     * Internal bridge callback from C++.
     */
    @:keep
    private static function _onNativeEvent(path:cpp.ConstCharStar, type:Int, isDir:Int):Void {
        if (_callback != null) {
            var eventType = switch (type) {
                case 1: Created;
                case 2: Modified;
                case 3: Deleted;
                case 4: Renamed;
                default: Modified;
            };

            _callback({
                path: path.toString(),
                type: eventType,
                isDir: isDir == 1
            });
        }
    }
}
