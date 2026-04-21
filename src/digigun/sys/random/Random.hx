package digigun.sys.random;

import haxe.io.Bytes;
import haxe.io.BytesData;

/**
 * Provides access to the system's cryptographically secure pseudo-random number generator (CSPRNG).
 */
class Random {
    /**
     * Generates a specified number of cryptographically secure random bytes.
     * @param length The number of bytes to generate.
     * @return A Bytes object containing the random data, or null on failure.
     */
    public static function getBytes(length:Int):Bytes {
        #if cpp
        var bytes = Bytes.alloc(length);
        var data:BytesData = bytes.getData();
        var ptr:cpp.RawPointer<cpp.UInt8> = cast cpp.NativeArray.address(data, 0);
        
        if (Native.get_bytes(ptr, length) == 0) {
            return bytes;
        }
        return null;
        #else
        return null;
        #end
    }

    /**
     * Generates a version 4 UUID (RFC 4122) using system entropy.
     * @return A UUID string.
     */
    public static function uuid():String {
        var bytes = getBytes(16);
        if (bytes == null) return null;

        // Set version (4) and variant (RFC 4122) bits
        var b6 = bytes.get(6);
        var b8 = bytes.get(8);
        bytes.set(6, (b6 & 0x0F) | 0x40);
        bytes.set(8, (b8 & 0x3F) | 0x80);

        var hex = bytes.toHex();
        return hex.substr(0, 8) + "-" + 
               hex.substr(8, 4) + "-" + 
               hex.substr(12, 4) + "-" + 
               hex.substr(16, 4) + "-" + 
               hex.substr(20);
    }
}
