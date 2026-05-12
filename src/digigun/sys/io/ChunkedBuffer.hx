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
class ChunkedBuffer implements IByteBuffer {
    public var handle(default, null):NativeHandle;

    public function new(chunkSize:Int = 4096) {
        handle = new NativeHandle(Native.create(chunkSize));
    }

    public function destroy():Void {
        if (handle.isValid) {
            Native.destroy(handle.value);
            handle = NativeHandle.nullHandle();
        }
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
        if (len <= 0) return 0;
        var ptr:RawPointer<cpp.Void> = cast untyped __cpp__("(void*)&{0}->b[{1}]", bytes, offset);
        return Native.peek(handle.value, ptr, len);
    }

    public function skip(len:Int):Int {
        return Native.read(handle.value, null, len);
    }

    public function clear():Void {
        Native.clear(handle.value);
    }

    public var capacity(get, never):Int;
    private function get_capacity():Int return -1; // Infinite capacity

    public var available(get, never):Int;
    private function get_available():Int return Native.available(handle.value);

    public var freeSpace(get, never):Int;
    private function get_freeSpace():Int return 2147483647; // Always space

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
        if (_buf.writeBytes(bytes, 0, 1) != 1) throw haxe.io.Error.Custom("Buffer write error");
    }
    override public function writeBytes(s:Bytes, pos:Int, len:Int):Int {
        return _buf.writeBytes(s, pos, len);
    }
}
