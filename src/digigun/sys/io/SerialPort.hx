package digigun.sys.io;

import digigun.sys.NativeHandle;
import digigun.sys.io.NativeBuffer;

/**
 * Serial Port Parity configuration.
 */
enum abstract SerialParity(Int) from Int to Int {
    var NONE = 0;
    var ODD = 1;
    var EVEN = 2;
}

@:keep
@:include("io_native.h")
private extern class Native {
    @:native("io_serial_open")
    static function open(portName:cpp.ConstCharStar, baudRate:Int, dataBits:Int, parity:Int, stopBits:Int):haxe.Int64;

    @:native("io_serial_read")
    static function read(handle:haxe.Int64, buffer:cpp.RawPointer<cpp.Void>, length:Int):Int;

    @:native("io_serial_write")
    static function write(handle:haxe.Int64, buffer:cpp.RawPointer<cpp.Void>, length:Int):Int;

    @:native("io_serial_close")
    static function close(handle:haxe.Int64):Void;
}

/**
 * Native Serial/UART communication.
 * Provides high-performance, zero-dependency access to hardware serial ports.
 */
class SerialPort {
    public var handle(default, null):NativeHandle;

    private function new(h:haxe.Int64) {
        this.handle = new NativeHandle(h);
    }

    /**
     * Opens a serial port.
     * @param portName Device path (e.g. "/dev/ttyUSB0", "COM3").
     * @param baudRate Baud rate (standard values: 9600, 115200, etc.).
     * @param dataBits Number of data bits (usually 8).
     * @param parity Parity mode.
     * @param stopBits Number of stop bits (1 or 2).
     * @return SerialPort instance or null if opening failed.
     */
    public static function open(portName:String, baudRate:Int = 9600, dataBits:Int = 8, parity:SerialParity = NONE, stopBits:Int = 1):SerialPort {
        var h = Native.open(portName, baudRate, dataBits, parity, stopBits);
        if (h != 0) {
            return new SerialPort(h);
        }
        return null;
    }

    /**
     * Reads bytes from the serial port into a native buffer.
     * @return Number of bytes read, or -1 on error.
     */
    public function read(buffer:NativeBuffer):Int {
        if (!handle.isValid) return -1;
        return Native.read(handle.value, buffer.getPointer(), buffer.size);
    }

    /**
     * Writes bytes to the serial port from a native buffer.
     * @param buffer The source buffer.
     * @param length Number of bytes to write (defaults to buffer size).
     * @return Number of bytes written, or -1 on error.
     */
    public function write(buffer:NativeBuffer, length:Int = -1):Int {
        if (!handle.isValid) return -1;
        if (length < 0) length = buffer.size;
        return Native.write(handle.value, buffer.getPointer(), length);
    }

    /**
     * Closes the serial port.
     */
    public function close():Void {
        if (handle.isValid) {
            Native.close(handle.value);
            handle = NativeHandle.nullHandle();
        }
    }
}
