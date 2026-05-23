package digigun.sys.io;

import digigun.sys.NativeHandle;
import haxe.io.Bytes;
import cpp.RawPointer;
import cpp.Star;

@:keep
@:include("buffer_native.h")
private extern class Native {
    @:native("bip_buffer_create")
    static function create(size:Int):haxe.Int64;

    @:native("bip_buffer_destroy")
    static function destroy(handle:haxe.Int64):Void;

    @:native("bip_buffer_reserve")
    static function reserve(handle:haxe.Int64, len:Int, reservedLen:Star<Int>):RawPointer<cpp.Void>;

    @:native("bip_buffer_commit")
    static function commit(handle:haxe.Int64, len:Int):Void;

    @:native("bip_buffer_get_read_ptr")
    static function getReadPtr(handle:haxe.Int64, availableLen:Star<Int>):RawPointer<cpp.Void>;

    @:native("bip_buffer_decommit")
    static function decommit(handle:haxe.Int64, len:Int):Void;

    @:native("bip_buffer_write")
    static function write(handle:haxe.Int64, data:RawPointer<cpp.Void>, len:Int):Int;

    @:native("bip_buffer_read")
    static function read(handle:haxe.Int64, data:RawPointer<cpp.Void>, len:Int):Int;

    @:native("bip_buffer_peek")
    static function peek(handle:haxe.Int64, data:RawPointer<cpp.Void>, len:Int):Int;

    @:native("bip_buffer_skip")
    static function skip(handle:haxe.Int64, len:Int):Int;

    @:native("bip_buffer_available")
    static function available(handle:haxe.Int64):Int;

    @:native("bip_buffer_free_space")
    static function freeSpace(handle:haxe.Int64):Int;

    @:native("bip_buffer_clear")
    static function clear(handle:haxe.Int64):Void;
}

/**
 * Result of a BipBuffer pointer operation.
 */
class BipPointer {
    public var address:haxe.Int64;
    public var len:Int;
    public function new(address:haxe.Int64, len:Int) {
        this.address = address;
        this.len = len;
    }
    @:noCompletion
    public var _ptr(get, never):RawPointer<cpp.Void>;
    private function get__ptr():RawPointer<cpp.Void> {
        return untyped __cpp__("(void*)(size_t){0}", address);
    }
}

/**
 * Bipartite circular buffer.
 * Guarantees contiguous memory blocks for read/write.
 */
class BipBuffer #if (!DIGIGUN_SYS_PURE_ALLOC && !digigun_sys_pure_alloc && !DIGIGUN.SYS.PURE_ALLOC && cpp) extends cpp.Finalizable #end implements IByteBuffer {
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

    /**
     * Reserves a contiguous block of memory for writing.
     */
    public function reserve(len:Int):BipPointer {
        if (_freed) return new BipPointer(0, 0);
        var resLen:Int = 0;
        var p:RawPointer<cpp.Void> = Native.reserve(handle.value, len, untyped __cpp__("&{0}", resLen));
        var addr:haxe.Int64 = untyped __cpp__("(long long)(size_t){0}", p);
        return new BipPointer(addr, resLen);
    }

    public function commit(len:Int):Void {
        if (!_freed) Native.commit(handle.value, len);
    }

    /**
     * Gets a pointer to a contiguous block of memory for reading.
     */
    public function getReadPtr():BipPointer {
        if (_freed) return new BipPointer(0, 0);
        var availLen:Int = 0;
        var p:RawPointer<cpp.Void> = Native.getReadPtr(handle.value, untyped __cpp__("&{0}", availLen));
        var addr:haxe.Int64 = untyped __cpp__("(long long)(size_t){0}", p);
        return new BipPointer(addr, availLen);
    }

    public function decommit(len:Int):Void {
        if (!_freed) Native.decommit(handle.value, len);
    }

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
    var b:BipBuffer;
    public function new(b:BipBuffer) { this.b = b; }
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
    var b:BipBuffer;
    public function new(b:BipBuffer) { this.b = b; }
    override public function writeByte(v:Int):Void {
        var bytes = Bytes.alloc(1);
        bytes.set(0, v);
        if (b.writeBytes(bytes, 0, 1) == 0) throw haxe.io.Error.Custom("Buffer full");
    }
    override public function writeBytes(s:Bytes, pos:Int, len:Int):Int {
        return b.writeBytes(s, pos, len);
    }
}
