import digigun.sys.fifo.Fifo;
import digigun.sys.fifo.UnixDomainSocket;
import digigun.sys.network.Network;
import digigun.sys.network.NetworkControl;
import digigun.sys.network.SocketOption;
import digigun.sys.network.PingSession;
import digigun.sys.process.Process;
import digigun.sys.shm.SharedMemory;
import digigun.sys.info.SystemInfo;
import digigun.sys.fs.Watcher;
import digigun.sys.fs.FileLock;
import digigun.sys.fs.MemoryMap;
import digigun.sys.fs.ExtendedAttributes;
import digigun.sys.sync.NamedSemaphore;
import digigun.sys.proc.ProcControl;
import digigun.sys.time.Time;
import digigun.sys.console.Console;
import digigun.sys.signal.Signal;
import digigun.sys.rt.RtControl;
import digigun.sys.random.Random;
import digigun.sys.auth.Auth;
import digigun.sys.service.Service;
import digigun.sys.dl.Dl;
import digigun.sys.dl.FFI;
import digigun.sys.io.NativeLoop;
import digigun.sys.io.NativeBuffer;
import digigun.sys.io.AsyncFile;
import sys.FileSystem;
import cpp.Callable;

using Main.FloatExt;

@:build(digigun.sys.dl.FFI.build())
class OutputLib {
    @:native("process_get_id")
    public static function getPid():Int return 0;

    @:native("process_echo")
    public static function echo(input:String):String return null;
}

@:build(digigun.sys.dl.FFI.struct())
class Point {
    public var x:Int;
    public var y:Int;
}

@:build(digigun.sys.dl.FFI.struct())
class ComplexData {
    public var id:Int;
    public var value:Float;
    public var name:String;
}

@:build(digigun.sys.dl.FFI.build())
class StructLib {
    @:native("process_point")
    public static function processPoint(p:Point):Void {}

    @:native("process_complex")
    public static function processComplex(d:ComplexData):Void {}
}

class Main {
    static function getTestPath(name:String):String {
        if (Sys.systemName() == "Windows") {
            var envTemp = Sys.getEnv("TEMP");
            if (envTemp != null && envTemp != "") {
                return envTemp + "\\" + name;
            }
            return "C:\\temp\\" + name;
        }
        return "/tmp/" + name;
    }

    static function main() {
        var args = Sys.args();
        if (args.indexOf("--digigun-child-fork") != -1) {
            trace('[FORK-CHILD] Process ID: ${Process.getId()}');
            Sys.sleep(1);
            Sys.exit(0);
        }

        trace("digigun.sys.hx Functional Test Suite");
        trace('Current PID: ${Process.getId()}');

        testSystemInfo();
        testTime();
        testAuth();
        testRandom();
        testProcess();
        testProcessTree();
        testProcControl();
        testFork();
        testSignal();
        testRtControl();
        testFileLock();
        testMemoryMap();
        testWatcher();
        testExtendedAttributes();
        testDirectIO();
        testFifo();
        testUnixDomainSocket();
        testNonBlockingFifo();
        testReadAll();
        testSelector();
        testSharedMemory();
        testNamedSemaphore();
        testService();
        testDl();
        testFfi();
        testNativeLoop();
        testAsyncFile();

        trace("All architecture and implementation checks complete.");
    }

    static function testAsyncFile() {
        trace("--- Testing Async File I/O (Phase 4) ---");
        var loop = new NativeLoop();
        var testPath = getTestPath("test_async_file.txt");
        var content = "This data was written asynchronously using native threads/IOCP.";
        
        try { if (FileSystem.exists(testPath)) FileSystem.deleteFile(testPath); } catch(e:Dynamic) {}

        // Test Async Write
        var file = AsyncFile.open(loop, testPath, true);
        if (file != null) {
            trace("  File opened for async write.");
            var buf = new NativeBuffer(content.length);
            buf.fromBytes(haxe.io.Bytes.ofString(content));
            
            var completed = false;
            file.write(buf, (result, bytes) -> {
                trace('  WRITE CALLBACK: Result=$result, Bytes=$bytes');
                completed = true;
            });

            loop.poll(1000);
            file.close();
            buf.free();

            if (completed) {
                trace("  Async Write: SUCCESS");
            } else {
                trace("  Async Write: FAILED (No callback)");
            }
        } else {
            trace("  FAILED: Could not open file for async write.");
        }

        // Test Async Read
        var fileRead = AsyncFile.open(loop, testPath, false);
        if (fileRead != null) {
            trace("  File opened for async read.");
            var completed = false;
            fileRead.read(content.length, (result, data) -> {
                var readBack = data.toBytes().toString();
                trace('  READ CALLBACK: Result=$result, Content="$readBack"');
                if (readBack == content) trace("  Async Read Content Verification: SUCCESS");
                else trace("  Async Read Content Verification: FAILED");
                completed = true;
                data.free();
            });

            loop.poll(1000);
            fileRead.close();

            if (completed) {
                trace("  Async Read: SUCCESS");
            } else {
                trace("  Async Read: FAILED (No callback)");
            }
        } else {
            trace("  FAILED: Could not open file for async read.");
        }

        loop.close();
        try { if (FileSystem.exists(testPath)) FileSystem.deleteFile(testPath); } catch(e:Dynamic) {}
    }

    static function testAuth() {
        trace("--- Testing Auth (Identity) ---");
        var current = Auth.getCurrentUser();
        if (current != null) {
            trace('  Current User: ${current.username}');
            var groups = Auth.getGroups();
            trace('  Listed ${groups.length} groups.');
        }
    }

    static function testRandom() {
        trace("--- Testing Random ---");
        var bytes = Random.getBytes(16);
        if (bytes != null) trace('  Secure bytes hex: ${bytes.toHex()}');
    }

    static function testSignal() {
        trace("--- Testing Signal Handling ---");
        Signal.trap(Signal.INT, (sig) -> {
            trace('  [SIGNAL] Received signal: $sig');
        });
        Signal.raise(Signal.INT);
        Sys.sleep(0.1);
    }

    static function testRtControl() {
        trace("--- Testing RtControl ---");
        RtControl.mlockall(true, false);
        RtControl.munlockall();
    }

    static function testFork() {
        trace("--- Testing Fork ---");
        #if !windows
        var pid = Process.fork();
        if (pid == 0) Sys.exit(0);
        #end
    }

    static function testTime() {
        trace("--- Testing Time ---");
        var start = Time.stamp();
        Sys.sleep(0.01);
        trace('  Elapsed: ${Time.stamp() - start}s');
    }

    static function testMemoryMap() {
        trace("--- Testing MemoryMap ---");
        var path = getTestPath("test_mmap.dat");
        sys.io.File.saveContent(path, "Memory Mapping is fast!");
        var mmap = MemoryMap.open(path, 24, true);
        if (mmap != null) mmap.close();
    }

    static function testProcControl() {
        trace("--- Testing ProcControl ---");
        ProcControl.getAffinity();
    }

    static function testNamedSemaphore() {
        trace("--- Testing NamedSemaphore ---");
        var sem = NamedSemaphore.open("/digigun_test_sem", 1);
        if (sem != null) {
            sem.close();
            sem.unlink();
        }
    }

    static function testFileLock() {
        trace("--- Testing FileLock ---");
        var path = getTestPath("test_lock.dat");
        sys.io.File.saveContent(path, "lock test");
        var lock = FileLock.lock(path, true, false);
        if (lock != null) lock.unlock();
    }

    static function testWatcher() {
        trace("--- Testing Watcher ---");
        var watchDir = getTestPath("test_watch_dir");
        if (!FileSystem.exists(watchDir)) FileSystem.createDirectory(watchDir);
        var success = Watcher.watch(watchDir, (event) -> {}, true);
        if (success) Watcher.stopAll();
    }

    static function testExtendedAttributes() {
        trace("--- Testing ExtendedAttributes ---");
        var path = getTestPath("test_xattr.dat");
        sys.io.File.saveContent(path, "xattr test");
        if (ExtendedAttributes.set(path, "user.test", haxe.io.Bytes.ofString("val"))) {
            ExtendedAttributes.remove(path, "user.test");
            trace("  ExtendedAttributes: SUCCESS");
        }
    }

    static function testSystemInfo() {
        trace("--- Testing SystemInfo ---");
        var mem = SystemInfo.getMemoryInfo();
        trace('  RAM Total: ${(mem.total/1024/1024/1024).format(2)} GB');
    }

    static function testSharedMemory() {
        trace("--- Testing SharedMemory ---");
        var shm = new SharedMemory();
        if (shm.open("/digigun_test_shm", 1024, true)) {
            shm.close();
            shm.unlink();
            trace("  SharedMemory: SUCCESS");
        }
    }

    static function testProcess() {
        trace("--- Testing Process ---");
        Process.isRoot();
    }

    static function testProcessTree() {
        trace("--- Testing ProcessTree ---");
        Process.getProcessTree(Process.getId());
    }

    static function testDirectIO() {
        trace("--- Testing DirectIO ---");
        var path = getTestPath("test_direct.dat");
        sys.io.File.saveContent(path, "direct io test data");
        var handle = digigun.sys.io.IO.openFile(path, false);
        if (handle.isValid) {
            digigun.sys.io.IO.setDirectIO(handle, true);
            digigun.sys.io.IO.closeHandle(handle);
            trace("  DirectIO: SUCCESS");
        }
    }

    static function testFifo() {
        trace("--- Testing Fifo ---");
        var fifoPath = getTestPath("haxe_fifo_test");
        var fifo = new Fifo();
        if (fifo.create(fifoPath)) trace("  Fifo: SUCCESS");
    }

    static function testUnixDomainSocket() {
        trace("--- Testing UnixDomainSocket ---");
        var sockPath = getTestPath("haxe_socket_test");
        var sock = new UnixDomainSocket();
        if (sock.bind(sockPath)) {
            sock.close();
            trace("  UnixDomainSocket: SUCCESS");
        }
    }

    static function testNonBlockingFifo() {
        trace("--- Testing NonBlocking Sockets ---");
        var sock = new UnixDomainSocket();
        sock.setBlocking(false);
        try { sock.accept(); } catch(e:Dynamic) {}
        sock.close();
        trace("  NonBlocking accept completed.");
    }

    static function testReadAll() {
        trace("--- Testing readAll ---");
        var sock = new UnixDomainSocket();
        sock.setBlocking(false);
        sock.readAll();
        sock.close();
        trace("  readAll completed.");
    }

    static function testSelector() {
        trace("--- Testing Selector ---");
        var sock = new UnixDomainSocket();
        sock.setBlocking(false);
        digigun.sys.fifo.Selector.select([sock], [], 0.01);
        sock.close();
        trace("  Selector: SUCCESS");
    }

    static function testService() {
        trace("--- Testing Service ---");
        Service.isAvailable();
    }

    static function testDl() {
        trace("--- Testing Dl ---");
        var libName = switch (Sys.systemName()) {
            case "Windows": "output.dll";
            case "Mac": "output.dylib";
            default: "output.so";
        };
        var libPath = "bin/lib/" + (Sys.systemName() == "Windows" ? "winarm/" : (Sys.systemName() == "Mac" ? "mac/" : "linux/")) + libName;
        var handle = Dl.open(libPath);
        if (handle.isValid) {
            trace("  Dl: SUCCESS");
            Dl.close(handle);
        }
    }

    static function testFfi() {
        trace("--- Testing FFI ---");
        var libName = switch (Sys.systemName()) {
            case "Windows": "output.dll";
            case "Mac": "output.dylib";
            default: "output.so";
        };
        var libPath = "bin/lib/" + (Sys.systemName() == "Windows" ? "winarm/" : (Sys.systemName() == "Mac" ? "mac/" : "linux/")) + libName;

        if (OutputLib.bind(libPath)) {
            trace("  Binding success.");
            var echoed = OutputLib.echo("Hello FFI");
            if (echoed != null) trace("  String auto-conversion: SUCCESS");

            if (StructLib.bind(libPath)) {
                var p = new Point();
                p.x = 10; p.y = 20;
                StructLib.processPoint(p);
                if (p.x == 110) trace("  Simple Struct Mapping: SUCCESS");

                var d = new ComplexData();
                d.id = 42; d.value = 3.14; d.name = "FFI Struct";
                StructLib.processComplex(d);
                if (d.id == 84) trace("  Complex Struct Mapping: SUCCESS");
            }
        }
    }

    static function testNativeLoop() {
        trace("--- Testing NativeLoop ---");
        var loop = new NativeLoop();
        if (loop.handle != null && loop.handle.isValid) {
            trace("  NativeLoop handle valid.");
            
            var fifoPath = getTestPath("test_loop_fifo");
            var fifo = new Fifo();
            try { if (FileSystem.exists(fifoPath)) FileSystem.deleteFile(fifoPath); } catch(e:Dynamic) {}

            if (fifo.create(fifoPath)) {
                trace("  FIFO created.");
                
                // Spawn writer thread BEFORE opening the reader to avoid deadlock on Windows
                trace("  Spawning writer thread...");
                sys.thread.Thread.create(() -> {
                    Sys.sleep(0.5);
                    var writer = new Fifo();
                    if (writer.open(fifoPath, true)) {
                        writer.write(haxe.io.Bytes.ofString("LoopData"), 8);
                        writer.close();
                    }
                });

                if (fifo.open(fifoPath, false)) { // blocks until writer connects
                    trace("  FIFO opened for async read.");
                    var readBuf = haxe.io.Bytes.alloc(64);
                    var completed = false;
                    loop.read(fifo.getHandle(), readBuf, (result, bytes) -> {
                        trace('  CALLBACK TRIGGERED: res=$result, bytes=$bytes');
                        if (bytes > 0) trace('  ASYNC READ: "${readBuf.getString(0, bytes)}"');
                        completed = true;
                    });

                    trace("  Polling loop...");
                    var events = loop.poll(2000);
                    trace('  Poll returned $events events.');
                    if (completed) trace("  NativeLoop: SUCCESS");
                    else trace("  NativeLoop: FAILED (Callback not triggered)");
                    fifo.close();
                } else {
                    trace("  FAILED: Could not open FIFO.");
                }
            } else {
                trace("  FAILED: Could not create FIFO.");
            }
            loop.close();
        }
    }
}

class FloatExt {
    public static function format(f:Float, precision:Int):String {
        return Std.string(Math.round(f * Math.pow(10, precision)) / Math.pow(10, precision));
    }
}
