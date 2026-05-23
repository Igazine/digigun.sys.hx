package digigun.sys.io;

import digigun.sys.NativeHandle;
import haxe.io.Bytes;
import cpp.RawPointer;

@:keep
@:include("buffer_native.h")
private extern class Native {
    @:native("ring_buffer_create")
    static function create(size:Int):haxe.Int64;

    @:native("ring_buffer_destroy")
    static function destroy(handle:haxe.Int64):Void;

    @:native("ring_buffer_write")
    static function write(handle:haxe.Int64, data:RawPointer<cpp.Void>, len:Int):Int;

    @:native("ring_buffer_read")
    static function read(handle:haxe.Int64, data:RawPointer<cpp.Void>, len:Int):Int;

    @:native("ring_buffer_peek")
    static function peek(handle:haxe.Int64, data:RawPointer<cpp.Void>, len:Int):Int;

    @:native("ring_buffer_skip")
    static function skip(handle:haxe.Int64, len:Int):Int;

    @:native("ring_buffer_available")
    static function available(handle:haxe.Int64):Int;

    @:native("ring_buffer_free_space")
    static function freeSpace(handle:haxe.Int64):Int;

    @:native("ring_buffer_clear")
    static function clear(handle:haxe.Int64):Void;
}

/**
 * Native-backed circular RingBuffer.
 */
class RingBuffer #if (!DIGIGUN_SYS_PURE_ALLOC && !digigun_sys_pure_alloc && !DIGIGUN.SYS.PURE_ALLOC && cpp) extends cpp.Finalizable #end implements IByteBuffer {
    public var handle(default, null):NativeHandle;
    private var _capacity:Int;
    private var _freed:Bool = false;

    public function new(size:Int) {
        #if (!DIGIGUN_SYS_PURE_ALLOC && !digigun_sys_pure_alloc && !DIGIGUN.SYS.PURE_ALLOC && cpp)
        super();
        #end
        _capacity = size;
        handle = new NativeHandle(Native.create(size));
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
        return Native.skip(handle.value, len);
    }

    public function clear():Void {
        if (!_freed) Native.clear(handle.value);
    }

    public var capacity(get, never):Int;
    private function get_capacity():Int return _capacity;

    public var available(get, never):Int;
    private function get_available():Int return _freed ? 0 : Native.available(handle.value);

    public var freeSpace(get, never):Int;
    private function get_freeSpace():Int return _freed ? 0 : Native.freeSpace(handle.value);

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
    var b:RingBuffer;
    public function new(b:RingBuffer) { this.b = b; }
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
    var b:RingBuffer;
    public function new(b:RingBuffer) { this.b = b; }
    override public function writeByte(v:Int):Void {
        var bytes = Bytes.alloc(1);
        bytes.set(0, v);
        if (b.writeBytes(bytes, 0, 1) == 0) throw haxe.io.Error.Custom("Buffer full");
    }
    override public function writeBytes(s:Bytes, pos:Int, len:Int):Int {
        return b.writeBytes(s, pos, len);
    }
}
