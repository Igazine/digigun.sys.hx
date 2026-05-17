package digigun.sys.io;

import digigun.sys.NativeHandle;
import cpp.Callable;
import cpp.RawPointer;
import cpp.Star;

@:include("loop_native.h")
@:native("loop_op_t")
extern enum abstract LoopOp(Int) from Int to Int {
    @:native("LOOP_OP_READ") var READ;
    @:native("LOOP_OP_WRITE") var WRITE;
    @:native("LOOP_OP_ACCEPT") var ACCEPT;
    @:native("LOOP_OP_CONNECT") var CONNECT;
}

typedef NativeLoopCallback = Callable<(userData:RawPointer<cpp.Void>, result:Int, bytesTransferred:Int)->Void>;

@:keep
@:include("loop_native.h")
private extern class Native {
    @:native("loop_create")
    static function create():haxe.Int64;

    @:native("loop_close")
    static function close(handle:haxe.Int64):Void;

    @:native("loop_submit")
    static function submit(handle:haxe.Int64, fd:haxe.Int64, op:Int, buffer:RawPointer<cpp.Void>, len:Int, callback:NativeLoopCallback, userData:RawPointer<cpp.Void>):Int;

    @:native("loop_poll")
    static function poll(handle:haxe.Int64, timeoutMs:Int):Int;
}

class NativeLoop {
    public var handle:NativeHandle;
    
    private static var _requests:Map<Int, (Int, Int)->Void> = new Map();
    private static var _nextId:Int = 1;
    private static var _nativeCallback:NativeLoopCallback = null;

    public function new() {
        digigun.sys.NativeBuild.init();
        handle = new NativeHandle(Native.create());
        if (_nativeCallback == null) {
            _nativeCallback = Callable.fromStaticFunction(_dispatch);
        }
    }

    public function close() {
        if (handle != null && handle.isValid) {
            Native.close(handle.value);
            handle = null;
        }
    }

    public function read(fd:NativeHandle, buffer:haxe.io.Bytes, callback:(result:Int, bytes:Int)->Void):Int {
        return submit(fd, READ, buffer, callback);
    }

    public function write(fd:NativeHandle, buffer:haxe.io.Bytes, callback:(result:Int, bytes:Int)->Void):Int {
        return submit(fd, WRITE, buffer, callback);
    }

    public function readNative(fd:NativeHandle, buffer:NativeBuffer, callback:(result:Int, bytes:Int)->Void):Int {
        return submitNative(fd, READ, buffer, callback);
    }

    public function writeNative(fd:NativeHandle, buffer:NativeBuffer, callback:(result:Int, bytes:Int)->Void):Int {
        return submitNative(fd, WRITE, buffer, callback);
    }

    public function submitNative(fd:NativeHandle, op:LoopOp, buffer:NativeBuffer, callback:(result:Int, bytes:Int)->Void):Int {
        var id = _nextId++;
        _requests.set(id, callback);
        
        var res = Native.submit(handle.value, fd.value, (op : Int), buffer._getPointer(), buffer.size, _nativeCallback, untyped __cpp__("(void*)(size_t){0}", id));
        
        if (res < 0) {
            _requests.remove(id);
        }
        return res;
    }

    public function submit(fd:NativeHandle, op:LoopOp, buffer:haxe.io.Bytes, callback:(result:Int, bytes:Int)->Void):Int {
        var id = _nextId++;
        _requests.set(id, callback);
        
        var ptr:RawPointer<cpp.Void> = cast untyped __cpp__("(void*)&{0}->b[0]", buffer);
        var res = Native.submit(handle.value, fd.value, (op : Int), ptr, buffer.length, _nativeCallback, untyped __cpp__("(void*)(size_t){0}", id));
        
        if (res < 0) {
            _requests.remove(id);
        }
        return res;
    }

    public function poll(timeoutMs:Int = -1):Int {
        return Native.poll(handle.value, timeoutMs);
    }

    private static function _dispatch(userData:RawPointer<cpp.Void>, result:Int, bytes:Int):Void {
        var id:Int = untyped __cpp__("(int)(size_t){0}", userData);
        var cb = _requests.get(id);
        if (cb != null) {
            cb(result, bytes);
            _requests.remove(id);
        }
    }
}
