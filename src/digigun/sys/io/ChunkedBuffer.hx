package digigun.sys.io;

import digigun.sys.NativeHandle;
import haxe.io.Bytes;
import cpp.RawPointer;

@:keep
@:include("buffer_native.h")
private extern class Native {
    @:native("chunked_buffer_create")
    static function create(chunkSize:Int):haxe.Int64;

    @:native("chunked_buffer_destroy")
    static function destroy(handle:haxe.Int64):Void;

    @:native("chunked_buffer_write")
    static function write(handle:haxe.Int64, data:RawPointer<cpp.Void>, len:Int):Int;

    @:native("chunked_buffer_read")
    static function read(handle:haxe.Int64, data:RawPointer<cpp.Void>, len:Int):Int;

    @:native("chunked_buffer_peek")
    static function peek(handle:haxe.Int64, data:RawPointer<cpp.Void>, len:Int):Int;

    @:native("chunked_buffer_available")
    static function available(handle:haxe.Int64):Int;

    @:native("chunked_buffer_clear")
    static function clear(handle:haxe.Int64):Void;
}

/**
 * Linked-list of native chunks.
 * Grows dynamically as needed.
 */
class ChunkedBuffer #if (!DIGIGUN_SYS_PURE_ALLOC && !digigun_sys_pure_alloc && !DIGIGUN.SYS.PURE_ALLOC && cpp) extends cpp.Finalizable #end implements IByteBuffer {
    public var handle(default, null):NativeHandle;
    private var _freed:Bool = false;
    private var _chunkSize:Int;

    public function new(chunkSize:Int = 4096) {
        #if (!DIGIGUN_SYS_PURE_ALLOC && !digigun_sys_pure_alloc && !DIGIGUN.SYS.PURE_ALLOC && cpp)
        super();
        #end
        this._chunkSize = chunkSize;
        handle = new NativeHandle(Native.create(chunkSize));
    }

    public function destroy():Void {
        if (!_freed && handle.isValid) {
            _freed = true;
            Native.destroy(handle.value);
        }
    }

    #if (!DIGIGUN_SYS_PURE_ALLOC && !digigun_sys_pure_alloc && !DIGIGUN.SYS.PURE_ALLOC && cpp)
    override public function finalize():Void {
        destroy();
    }
    #end

    public function writeBytes(bytes:Bytes, offset:Int, len:Int):Int {
        if (_freed || len <= 0) return 0;
        var ptr:RawPointer<cpp.Void> = cast untyped __cpp__("(void*)&{0}->b[{1}]", bytes, offset);
        return Native.write(handle.value, ptr, len);
    }

    public function readBytes(bytes:Bytes, offset:Int, len:Int):Int {
        if (_freed || len <= 0) return 0;
        var ptr:RawPointer<cpp.Void> = cast untyped __cpp__("(void*)&{0}->b[{1}]", bytes, offset);
        return Native.read(handle.value, ptr, len);
    }

    public function peekBytes(bytes:Bytes, offset:Int, len:Int):Int {
        if (_freed || len <= 0) return 0;
        var ptr:RawPointer<cpp.Void> = cast untyped __cpp__("(void*)&{0}->b[{1}]", bytes, offset);
        return Native.peek(handle.value, ptr, len);
    }

    public function skip(len:Int):Int {
        if (_freed) return 0;
        // ChunkedBuffer doesn't have a native skip, implement via read with NULL dest
        return Native.read(handle.value, null, len);
    }

    public var available(get, never):Int;
    private function get_available():Int return _freed ? 0 : Native.available(handle.value);

    public var capacity(get, never):Int;
    private function get_capacity():Int return -1; // Dynamic capacity

    public var freeSpace(get, never):Int;
    private function get_freeSpace():Int return 2147483647; // Effectively infinite

    public function clear():Void {
        if (!_freed) Native.clear(handle.value);
    }

    /**
     * Returns a Haxe Input wrapper.
     */
    public function asInput():haxe.io.Input {
        return new BufferInput(this);
    }

    /**
     * Returns a Haxe Output wrapper.
     */
    public function asOutput():haxe.io.Output {
        return new BufferOutput(this);
    }
}

private class BufferInput extends haxe.io.Input {
    var b:ChunkedBuffer;
    public function new(b:ChunkedBuffer) { this.b = b; }
    override public function readByte():Int {
        var bytes = Bytes.alloc(1);
        if (b.readBytes(bytes, 0, 1) == 0) throw new haxe.io.Eof();
        return bytes.get(0);
    }
    override public function readBytes(s:Bytes, pos:Int, len:Int):Int {
        return b.readBytes(s, pos, len);
    }
}

private class BufferOutput extends haxe.io.Output {
    var b:ChunkedBuffer;
    public function new(b:ChunkedBuffer) { this.b = b; }
    override public function writeByte(v:Int):Void {
        var bytes = Bytes.alloc(1);
        bytes.set(0, v);
        if (b.writeBytes(bytes, 0, 1) == 0) throw haxe.io.Error.Custom("Buffer full");
    }
    override public function writeBytes(s:Bytes, pos:Int, len:Int):Int {
        return b.writeBytes(s, pos, len);
    }
}
