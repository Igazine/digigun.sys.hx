package digigun.sys;

import haxe.Int64;

/**
 * A container for a native system resource handle.
 * Uses Int64 to represent the handle value, ensuring stability
 * across all hxcpp platforms and avoiding size_t template ambiguities.
 */
#if cpp
class NativeHandle {
    public var value(default, null):cpp.SizeT;

    public function new(val:cpp.SizeT) {
        this.value = val;
    }

    /**
     * Checks if the handle is valid (not zero).
     */
    public var isValid(get, never):Bool;
    private inline function get_isValid():Bool {
        return value != 0;
    }

    /**
     * Returns a null/invalid handle.
     */
    public static function nullHandle():NativeHandle {
        return new NativeHandle(0);
    }

    /**
     * Explicit conversion to a raw pointer.
     */
    public inline function toVoidPtr():cpp.RawPointer<cpp.Void> {
        return untyped __cpp__("(void*){0}", value);
    }

    /**
     * Explicit creation from a raw pointer.
     */
    public static function fromVoidPtr(ptr:cpp.RawPointer<cpp.Void>):NativeHandle {
        return new NativeHandle(untyped __cpp__("(size_t){0}", ptr));
    }
}
#else
class NativeHandle {
    public var isValid(get, never):Bool;
    private function get_isValid():Bool return false;
    public static function nullHandle():NativeHandle return new NativeHandle();
    private function new() {}
}
#end
