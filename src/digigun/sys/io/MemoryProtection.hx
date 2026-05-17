package digigun.sys.io;

/**
 * Direct memory protection flags.
 */
enum abstract MemProt(Int) from Int to Int {
    /** No access allowed. */
    var NONE  = 0;
    /** Read access. */
    var READ  = 1;
    /** Write access. */
    var WRITE = 2;
    /** Execute access (for JIT/generated code). */
    var EXEC  = 4;
    
    /** Read and Write access. */
    var READ_WRITE = 3;
    /** Read and Execute access. */
    var READ_EXEC  = 5;
    /** Read, Write, and Execute access. */
    var ALL        = 7;
}

@:keep
@:include("io_native.h")
private extern class Native {
    @:native("io_mem_protect")
    static function protect(addr:cpp.RawPointer<cpp.Void>, len:Int, flags:Int):Int;

    @:native("io_mem_pagesize")
    static function getPageSize():Int;
}

/**
 * Direct control over memory page protection.
 */
class MemoryProtection {
    /**
     * Changes the access protection for a region of memory by address.
     * @param address Raw memory address.
     * @param length Size of the region in bytes.
     * @param flags Protection flags (bitmask of MemProt values).
     * @return True if successful.
     */
    public static function protectAddress(address:haxe.Int64, length:Int, flags:Int):Bool {
        var ptr:cpp.RawPointer<cpp.Void> = untyped __cpp__("(void*)(size_t){0}", address);
        return Native.protect(ptr, length, flags) == 0;
    }

    /**
     * Changes the access protection for a NativeBuffer.
     */
    public static function protect(buffer:NativeBuffer, flags:Int):Bool {
        return protectAddress(buffer.address, buffer.size, flags);
    }

    /**
     * Retrieves the system's memory page size.
     */
    public static function getPageSize():Int {
        return Native.getPageSize();
    }
}
