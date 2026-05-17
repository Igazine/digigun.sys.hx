package digigun.sys.io;

import digigun.sys.NativeHandle;
import haxe.io.Bytes;

@:keep
@:include("io_native.h")
private extern class Native {
    @:native("buffer_alloc")
    static function alloc(size:Int):haxe.Int64;

    @:native("buffer_free")
    static function free(handle:haxe.Int64):Void;

    @:native("buffer_get_ptr")
    static function getPtr(handle:haxe.Int64):cpp.RawPointer<cpp.Void>;
}

/**
 * A raw native memory buffer outside the Haxe GC.
 */
class NativeBuffer {
    public var handle(default, null):NativeHandle;
    public var size(default, null):Int;
    private var ownsHandle:Bool = true;

    public function new(size:Int) {
        this.size = size;
        this.handle = new NativeHandle(Native.alloc(size));
    }

    /**
     * Creates a NativeBuffer from an existing memory address.
     * The resulting buffer does not own the memory and will not free it.
     */
    public static function fromAddress(address:haxe.Int64, size:Int):NativeBuffer {
        var buf = Type.createEmptyInstance(NativeBuffer);
        buf.size = size;
        buf.handle = new NativeHandle(address);
        buf.ownsHandle = false;
        return buf;
    }

    public function free():Void {
        if (handle.isValid && ownsHandle) {
            Native.free(handle.value);
            handle = NativeHandle.nullHandle();
        }
    }

    /**
     * Returns the raw memory address of this buffer.
     */
    public var address(get, never):haxe.Int64;
    private inline function get_address():haxe.Int64 {
        return untyped __cpp__("(long long)(size_t){0}", getPointer());
    }

    @:noCompletion
    public function getPointer():cpp.RawPointer<cpp.Void> {
        return handle.isValid ? Native.getPtr(handle.value) : null;
    }

    /**
     * Copies data from this native buffer to a Haxe Bytes object.
     */
    public function toBytes():Bytes {
        if (!handle.isValid) return null;
        var bytes = Bytes.alloc(size);
        var dst:cpp.RawPointer<cpp.Void> = cast untyped __cpp__("(void*)&{0}->b[0]", bytes);
        untyped __cpp__("memcpy({0}, {1}, {2})", dst, getPointer(), size);
        return bytes;
    }

    /**
     * Copies data from a Haxe Bytes object to this native buffer.
     */
    public function fromBytes(bytes:Bytes, offset:Int = 0, length:Int = -1):Void {
        if (!handle.isValid) return;
        if (length == -1) length = bytes.length - offset;
        if (length > size) length = size;
        
        var src:cpp.RawPointer<cpp.Void> = cast untyped __cpp__("(void*)&{0}->b[{1}]", bytes, offset);
        untyped __cpp__("memcpy({0}, {1}, {2})", getPointer(), src, length);
    }
}
