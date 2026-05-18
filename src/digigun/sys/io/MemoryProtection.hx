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

#if !macro
@:keep
@:include("io_native.h")
private extern class Native {
    private static function __init__():Void { digigun.sys.NativeBuild.init(); }

    @:native("io_mem_protect")
    static function protect(addr:cpp.RawPointer<cpp.Void>, len:Int, flags:Int):Int;

    @:native("io_mem_pagesize")
    static function getPageSize():Int;
}
#end

/**
 * Direct control over memory page protection.
 */
class MemoryProtection {
    /**
     * Internal: Changes the access protection for a region of memory by address.
     */
    @:allow(digigun.sys)
    private static function _protectAddress(address:haxe.Int64, length:Int, flags:Int):Bool {
        #if !macro
        var ptr:cpp.RawPointer<cpp.Void> = untyped __cpp__("(void*)(size_t){0}", address);
        return Native.protect(ptr, length, flags) == 0;
        #else
        return false;
        #end
    }

    /**
     * Changes the access protection for a region of memory by address.
     * @param address Raw memory address.
     * @param length Size of the region in bytes.
     * @param flags Protection flags (bitmask of MemProt values).
     * @return True if successful.
     */
    #if (DIGIGUN.SYS.UNSAFE_MEMORY_ACCESS || digigun.sys.unsafe_memory_access || DIGIGUN_SYS_UNSAFE_MEMORY_ACCESS)
    public static inline function protectAddress(address:haxe.Int64, length:Int, flags:Int):Bool {
        return _protectAddress(address, length, flags);
    }
    #else
    public static macro function protectAddress(address:haxe.macro.Expr, length:haxe.macro.Expr, flags:haxe.macro.Expr):haxe.macro.Expr {
        haxe.macro.Context.error("Direct memory access (protectAddress) requires compiling with -D DIGIGUN.SYS.UNSAFE_MEMORY_ACCESS", haxe.macro.Context.currentPos());
        return macro false;
    }
    #end

    /**
     * Changes the access protection for a NativeBuffer.
     */
    public static function protect(buffer:NativeBuffer, flags:Int):Bool {
        return _protectAddress(buffer.address, buffer.size, flags);
    }

    /**
     * Retrieves the system's memory page size.
     */
    public static function getPageSize():Int {
        #if !macro
        return Native.getPageSize();
        #else
        return 4096;
        #end
    }
}
