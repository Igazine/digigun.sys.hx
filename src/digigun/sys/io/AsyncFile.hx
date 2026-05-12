package digigun.sys.io;

import digigun.sys.NativeHandle;
import haxe.io.Bytes;
import haxe.Int64;

/**
 * Completion-based asynchronous file operations.
 */
class AsyncFile {
    private var _loop:NativeLoop;
    private var _handle:NativeHandle;

    private function new(loop:NativeLoop, handle:NativeHandle) {
        this._loop = loop;
        this._handle = handle;
    }

    /**
     * Opens a file for asynchronous operations.
     */
    public static function open(loop:NativeLoop, path:String, writeMode:Bool = false):AsyncFile {
        var h = IO.openFile(path, writeMode);
        if (h.isValid) {
            return new AsyncFile(loop, h);
        }
        return null;
    }

    /**
     * Closes the file.
     */
    public function close():Void {
        if (_handle.isValid) {
            IO.closeHandle(_handle);
            _handle = NativeHandle.nullHandle();
        }
    }

    /**
     * Performs an asynchronous read from the file.
     */
    public function read(length:Int, callback:(result:Int, data:NativeBuffer)->Void):Void {
        var buffer = new NativeBuffer(length);
        _loop.readNative(_handle, buffer, (result, bytes) -> {
            callback(result, buffer);
        });
    }

    /**
     * Performs an asynchronous write to the file.
     */
    public function write(data:NativeBuffer, callback:(result:Int, bytes:Int)->Void):Void {
        _loop.writeNative(_handle, data, callback);
    }
}
