package digigun.sys.fifo;

import haxe.io.Bytes;

/**
 * Base interface for read/write operations on FIFOs and Unix Sockets.
 */
interface ICommunicator {
    /**
     * Reads up to `length` bytes from the communicator into the provided `buffer`.
     * @param buffer The buffer to read into.
     * @param length The maximum number of bytes to read.
     * @return The number of bytes actually read, or -1 on error.
     */
    function read(buffer:Bytes, length:Int):Int;

    /**
     * Reads all available data from the communicator.
     * @return The bytes read.
     */
    function readAll():Bytes;

    /**
     * Writes `length` bytes from the `buffer` to the communicator.
     * @param buffer The buffer containing data to write.
     * @param length The number of bytes to write.
     * @return The number of bytes actually written, or -1 on error.
     */
    function write(buffer:Bytes, length:Int):Int;

    /**
     * Toggles non-blocking mode for the communicator.
     * @param blocking True to enable blocking (default), false for non-blocking.
     * @return True if successful.
     */
    function setBlocking(blocking:Bool):Bool;

    /**
     * Returns the underlying native resource handle.
     */
    function getHandle():NativeHandle;

    /**
     * Closes the communicator.
     */
    function close():Void;
}
