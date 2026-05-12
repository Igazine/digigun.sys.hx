package digigun.sys.io;

import haxe.io.Bytes;

/**
 * Unified interface for high-performance native buffers.
 */
interface IByteBuffer {
    /**
     * Writes data from Haxe Bytes to the buffer.
     * Returns number of bytes actually written.
     */
    function writeBytes(bytes:Bytes, offset:Int, len:Int):Int;

    /**
     * Reads data from the buffer into Haxe Bytes.
     * Returns number of bytes actually read.
     */
    function readBytes(bytes:Bytes, offset:Int, len:Int):Int;

    /**
     * Peeks data without advancing the read pointer.
     */
    function peekBytes(bytes:Bytes, offset:Int, len:Int):Int;

    /**
     * Advances the read pointer without copying data.
     */
    function skip(len:Int):Int;

    /**
     * Clears all data in the buffer.
     */
    function clear():Void;

    /**
     * Total capacity of the buffer.
     */
    var capacity(get, never):Int;

    /**
     * Bytes currently available for reading.
     */
    var available(get, never):Int;

    /**
     * Space currently available for writing.
     */
    var freeSpace(get, never):Int;
}
