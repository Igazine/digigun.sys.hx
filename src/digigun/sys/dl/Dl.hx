package digigun.sys.dl;

import digigun.sys.NativeHandle;

/**
 * Provides cross-platform runtime loading of shared libraries.
 */
class Dl {
    /**
     * Loads a shared library from the specified path.
     * @param path File path to the library (.dylib, .so, .dll).
     * @return A NativeHandle to the library, or an invalid handle on failure.
     */
    public static function open(path:String):NativeHandle {
        #if cpp
        return new NativeHandle(Native.dl_open(path));
        #else
        return NativeHandle.nullHandle();
        #end
    }

    /**
     * Retrieves the address of an exported symbol from the library.
     * @param handle Handle to the loaded library.
     * @param name Name of the symbol to find.
     * @return A NativeHandle to the symbol address.
     */
    public static function getSymbol(handle:NativeHandle, name:String):NativeHandle {
        #if cpp
        if (!handle.isValid) return NativeHandle.nullHandle();
        return new NativeHandle(Native.dl_get_symbol(handle.value, name));
        #else
        return NativeHandle.nullHandle();
        #end
    }

    /**
     * Unloads the specified shared library.
     * @param handle Handle to the library to close.
     */
    public static function close(handle:NativeHandle):Void {
        #if cpp
        if (handle.isValid) {
            Native.dl_close(handle.value);
        }
        #end
    }

    /**
     * Returns the last error reported by the dynamic loader.
     */
    public static function getError():String {
        #if cpp
        var err = Native.dl_get_error();
        return (err != null) ? err.toString() : null;
        #else
        return "Not supported on this platform";
        #end
    }
}
