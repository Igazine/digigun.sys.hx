package digigun.sys.fifo;

import cpp.RawPointer;
import haxe.io.Bytes;
import haxe.io.BytesData;
import digigun.sys.NativeHandle;

/**
 * Concrete implementation of a Local Unix Socket using native system calls.
 */
class UnixDomainSocket implements ISocket {
    private var handle:NativeHandle;
    private var path:String;

    public function new(?h:NativeHandle) {
        this.handle = (h != null) ? h : NativeHandle.nullHandle();
    }

    public static function create():UnixDomainSocket {
        var sock = new UnixDomainSocket();
        #if cpp
        sock.handle = new NativeHandle(Native.socket_create());
        #end
        return sock;
    }

    public function bind(path:String):Bool {
        #if cpp
        if (!this.handle.isValid) return false;
        this.path = path;
        return Native.socket_bind(this.handle.value, path) == 0;
        #else
        return false;
        #end
    }

    public function listen(backlog:Int = 5):Bool {
        #if cpp
        if (!this.handle.isValid) return false;
        return Native.socket_listen(this.handle.value, backlog) == 0;
        #else
        return false;
        #end
    }

    public function accept():ISocket {
        #if cpp
        if (!this.handle.isValid) return null;
        var clientVal = Native.socket_accept(this.handle.value);
        if (clientVal == (cast -2 : cpp.SizeT)) throw haxe.io.Error.Blocked;
        var clientHandle = new NativeHandle(clientVal);
        if (!clientHandle.isValid) return null;
        return new UnixDomainSocket(clientHandle);
        #else
        return null;
        #end
    }

    public function connect(path:String):Bool {
        #if cpp
        if (!this.handle.isValid) return false;
        this.path = path;
        var result = Native.socket_connect(this.handle.value, path);
        if (result == -2) throw haxe.io.Error.Blocked;
        return result == 0;
        #else
        return false;
        #end
    }

    public function read(buffer:Bytes, length:Int):Int {
        #if cpp
        if (!this.handle.isValid) return -1;
        if (buffer.length < length) length = buffer.length;
        var data:BytesData = buffer.getData();
        var ptr:cpp.RawPointer<cpp.Char> = cast cpp.NativeArray.address(data, 0);
        var res = Native.fd_read(this.handle.value, ptr, length);
        if (res == -2) throw haxe.io.Error.Blocked;
        return res;
        #else
        return -1;
        #end
    }

    /**
     * Reads all available data from the socket.
     * In blocking mode, this will read until EOF.
     * In non-blocking mode, this will read all currently available data.
     */
    public function readAll():Bytes {
        var buffer = new haxe.io.BytesBuffer();
        var temp = Bytes.alloc(4096);
        try {
            while (true) {
                var res = read(temp, temp.length);
                if (res > 0) {
                    buffer.addBytes(temp, 0, res);
                } else if (res == 0) {
                    break; // EOF
                } else {
                    break; // Error
                }
            }
        } catch (e:haxe.io.Error) {
            if (e != haxe.io.Error.Blocked) throw e;
        }
        return buffer.getBytes();
    }

    public function write(buffer:Bytes, length:Int):Int {
        #if cpp
        if (!this.handle.isValid) return -1;
        if (buffer.length < length) length = buffer.length;
        var data:BytesData = buffer.getData();
        var ptr:cpp.RawConstPointer<cpp.Char> = cast cpp.NativeArray.address(data, 0);
        var res = Native.fd_write(this.handle.value, ptr, length);
        if (res == -2) throw haxe.io.Error.Blocked;
        return res;
        #else
        return -1;
        #end
    }

    public function setBlocking(blocking:Bool):Bool {
        #if cpp
        if (!this.handle.isValid) return false;
        return Native.fd_set_blocking(this.handle.value, blocking ? 1 : 0) == 0;
        #else
        return false;
        #end
    }

    public function getHandle():NativeHandle {
        return this.handle;
    }

    public function close():Void {
        #if cpp
        if (this.handle.isValid) {
            Native.fd_close(this.handle.value);
            this.handle = NativeHandle.nullHandle();
        }
        #end
    }
}
