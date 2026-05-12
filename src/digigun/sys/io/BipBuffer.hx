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
    public var ptr(get, never):RawPointer<cpp.Void>;
    private function get_ptr():RawPointer<cpp.Void> {
        return untyped __cpp__("(void*)(size_t){0}", address);
    }
}

/**
 * Bipartite circular buffer.
 * Guarantees contiguous memory blocks for read/write.
 */
class BipBuffer implements IByteBuffer {
    public var handle(default, null):NativeHandle;
    private var _capacity:Int;

    public function new(size:Int) {
        _capacity = size;
        handle = new NativeHandle(Native.create(size));
    }

    public function destroy():Void {
        if (handle.isValid) {
            Native.destroy(handle.value);
            handle = NativeHandle.nullHandle();
        }
    }

    /**
     * Reserves a contiguous block of memory for writing.
     */
    public function reserve(len:Int):BipPointer {
        var resLen:Int = 0;
        var p:RawPointer<cpp.Void> = Native.reserve(handle.value, len, untyped __cpp__("&{0}", resLen));
        var addr:haxe.Int64 = untyped __cpp__("(long long)(size_t){0}", p);
        return new BipPointer(addr, resLen);
    }

    public function commit(len:Int):Void {
        Native.commit(handle.value, len);
    }

    /**
     * Gets a pointer to a contiguous block of memory for reading.
     */
    public function getReadPtr():BipPointer {
        var availLen:Int = 0;
        var p:RawPointer<cpp.Void> = Native.getReadPtr(handle.value, untyped __cpp__("&{0}", availLen));
        var addr:haxe.Int64 = untyped __cpp__("(long long)(size_t){0}", p);
        return new BipPointer(addr, availLen);
    }

    public function decommit(len:Int):Void {
        Native.decommit(handle.value, len);
    }

    public function writeBytes(bytes:Bytes, offset:Int, len:Int):Int {
        if (len <= 0) return 0;
        var ptr:RawPointer<cpp.Void> = cast untyped __cpp__("(void*)&{0}->b[{1}]", bytes, offset);
        return Native.write(handle.value, ptr, len);
    }

    public function readBytes(bytes:Bytes, offset:Int, len:Int):Int {
        if (len <= 0) return 0;
        var ptr:RawPointer<cpp.Void> = cast untyped __cpp__("(void*)&{0}->b[{1}]", bytes, offset);
        return Native.read(handle.value, ptr, len);
    }

    public function peekBytes(bytes:Bytes, offset:Int, len:Int):Int {
        var availLen:Int = 0;
        var src:RawPointer<cpp.Void> = Native.getReadPtr(handle.value, untyped __cpp__("&{0}", availLen));
        if (src == null || availLen <= 0) return 0;
        
        var toPeek = Std.int(Math.min(len, availLen));
        var dst:RawPointer<cpp.Void> = cast untyped __cpp__("(void*)&{0}->b[{1}]", bytes, offset);
        untyped __cpp__("memcpy({0}, {1}, {2})", dst, src, toPeek);
        return toPeek;
    }

    public function skip(len:Int):Int {
        var avail = available;
        var toSkip = Std.int(Math.min(len, avail));
        decommit(toSkip);
        return toSkip;
    }

    public function clear():Void {
        Native.clear(handle.value);
    }

    public var capacity(get, never):Int;
    private function get_capacity():Int return _capacity;

    public var available(get, never):Int;
    private function get_available():Int return Native.available(handle.value);

    public var freeSpace(get, never):Int;
    private function get_freeSpace():Int return Native.freeSpace(handle.value);

    public function asInput():haxe.io.Input {
        return new BufferInput(this);
    }

    public function asOutput():haxe.io.Output {
        return new BufferOutput(this);
    }
}

/**
 * Internal Haxe Input wrapper for IByteBuffer.
 */
private class BufferInput extends haxe.io.Input {
    private var _buf:IByteBuffer;
    public function new(buf:IByteBuffer) { this._buf = buf; }
    override public function readByte():Int {
        var b = Bytes.alloc(1);
        if (_buf.readBytes(b, 0, 1) == 1) return b.get(0);
        throw new haxe.io.Eof();
    }
    override public function readBytes(s:Bytes, pos:Int, len:Int):Int {
        return _buf.readBytes(s, pos, len);
    }
}

private class BufferOutput extends haxe.io.Output {
    private var _buf:IByteBuffer;
    public function new(buf:IByteBuffer) { this._buf = buf; }
    override public function writeByte(b:Int):Void {
        var bytes = Bytes.alloc(1);
        bytes.set(0, b);
        if (_buf.writeBytes(bytes, 0, 1) != 1) throw haxe.io.Error.Custom("Buffer full");
    }
    override public function writeBytes(s:Bytes, pos:Int, len:Int):Int {
        return _buf.writeBytes(s, pos, len);
    }
}
