import digigun.sys.fifo.Fifo;
import digigun.sys.fifo.UnixDomainSocket;
import digigun.sys.network.Network;
import digigun.sys.network.NetworkControl;
import digigun.sys.network.SocketOption;
import digigun.sys.network.PingSession;
import digigun.sys.network.RawSocket;
import digigun.sys.process.Process;
import digigun.sys.process.PriorityClass;
import digigun.sys.shm.SharedMemory;
import digigun.sys.info.SystemInfo;
import digigun.sys.fs.Watcher;
import digigun.sys.fs.FileLock;
import digigun.sys.fs.MemoryMap;
import digigun.sys.fs.ExtendedAttributes;
import digigun.sys.fs.Symlink;
import digigun.sys.fs.FileDiagnostics;
import digigun.sys.fs.Volume;
import digigun.sys.sync.NamedSemaphore;
import digigun.sys.sync.Futex;
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
import digigun.sys.io.MemoryProtection;
import digigun.sys.io.SerialPort;
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
            Process.exitWithParent();
            Sys.exit(0);
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
        testSymlink();
        testFileDiagnostics();
        testVolume();
        testExtendedAttributes();
        testDirectIO();
        testFifo();
        testUnixDomainSocket();
        testNonBlockingFifo();
        testReadAll();
        testSelector();
        testSharedMemory();
        testNamedSemaphore();
        testFutex();
        testService();
        testDl();
        testFfi();
        testNativeLoop();
        testAsyncFile();
        testAdvancedBuffers();
        testMemoryProtection();
        testSerialPort();
        testRawSocket();
        
        var args = Sys.args();
        if (args.indexOf("--crash-test") != -1) {
            testCrashHandler();
        }

        trace("All architecture and implementation checks complete.");
    }

    static function testAdvancedBuffers() {
        trace("--- Testing Advanced Native Buffers (Phase 6) ---");
        
        // 1. RingBuffer Test
        var rb = new digigun.sys.io.RingBuffer(1024);
        var data1 = haxe.io.Bytes.ofString("RingBufferData");
        rb.writeBytes(data1, 0, data1.length);
        var read1 = haxe.io.Bytes.alloc(data1.length);
        rb.readBytes(read1, 0, read1.length);
        if (read1.toString() == data1.toString()) trace("  RingBuffer: SUCCESS");
        else trace("  RingBuffer: FAILED");
        rb.destroy();

        // 2. BipBuffer Test
        var bb = new digigun.sys.io.BipBuffer(1024);
        var data2 = haxe.io.Bytes.ofString("BipBufferData");
        bb.writeBytes(data2, 0, data2.length);
        
        // Test contiguous access
        var readInfo:digigun.sys.io.BipBuffer.BipPointer = bb.getReadPtr();
        if (readInfo.address != 0 && readInfo.len >= data2.length) {
            var read2 = haxe.io.Bytes.alloc(data2.length);
            // Copy from native address to verify
            var dst:cpp.RawPointer<cpp.Void> = cast untyped __cpp__("(void*)&{0}->b[0]", read2);
            untyped __cpp__("memcpy({0}, (void*)(size_t){1}, {2})", dst, readInfo.address, data2.length);
            if (read2.toString() == data2.toString()) trace("  BipBuffer Contiguous Read: SUCCESS");
        }
        bb.destroy();

        // 3. ChunkedBuffer Test
        var cb = new digigun.sys.io.ChunkedBuffer(32); // small chunks
        var largeData = haxe.io.Bytes.alloc(100);
        for (i in 0...100) largeData.set(i, i);
        cb.writeBytes(largeData, 0, 100);
        trace('  ChunkedBuffer available: ${cb.available}');
        var read3 = haxe.io.Bytes.alloc(100);
        cb.readBytes(read3, 0, 100);
        var success3 = true;
        for (i in 0...100) if (read3.get(i) != i) success3 = false;
        if (success3) trace("  ChunkedBuffer: SUCCESS");
        else trace("  ChunkedBuffer: FAILED");
        cb.destroy();

        // 4. Input/Output Test
        var rb2 = new digigun.sys.io.RingBuffer(1024);
        var output = rb2.asOutput();
        output.writeString("IOTest");
        var input = rb2.asInput();
        if (input.readString(6) == "IOTest") trace("  Buffer I/O Wrappers: SUCCESS");
        else trace("  Buffer I/O Wrappers: FAILED");
        rb2.destroy();
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
        trace("--- Testing Process Control (Affinity/Priority) ---");
        Process.getAffinity();
        Process.setPriority(Normal);
    }

    static function testNamedSemaphore() {
        trace("--- Testing NamedSemaphore ---");
        var sem = NamedSemaphore.open("/digigun_test_sem", 1);
        if (sem != null) {
            sem.close();
            sem.unlink();
        }
    }

    static function testFutex() {
        trace("--- Testing Futex / WaitOnAddress ---");
        // Use NativeBuffer (malloc) to ensure memory never moves and is correctly aligned
        var mem = new digigun.sys.io.NativeBuffer(4);
        untyped __cpp__("*(int*)(size_t){0} = 0", mem.address);

        trace("  Spawning waker thread...");
        sys.thread.Thread.create(() -> {
            Sys.sleep(0.5);
            trace("  [WAKER] Changing value to 42 and waking...");
            untyped __cpp__("*(int*)(size_t){0} = 42", mem.address);
            var woken = Futex.wake(mem, 0);
            trace('  [WAKER] Wake call returned: $woken');
        });

        trace("  [MAIN] Waiting on futex (expected=0)...");
        var start = Time.stamp();
        // Blocks ONLY if value == 0
        if (Futex.wait(mem, 0, 0)) {
            var elapsed = Time.stamp() - start;
            var finalVal:Int = untyped __cpp__("*(int*)(size_t){0}", mem.address);
            trace('  [MAIN] Awoken! Elapsed: ${elapsed}s, Value: $finalVal');
            if (finalVal == 42 && elapsed >= 0.4) {
                trace("  Futex synchronization: SUCCESS");
            } else {
                trace("  Futex synchronization: FAILED (Early wake or wrong value)");
            }
        } else {
            trace("  Futex wait: FAILED");
        }
        mem.free();
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

    static function testSymlink() {
        trace("--- Testing Symbolic Links ---");
        var targetPath = getTestPath("symlink_target.txt");
        var linkPath = getTestPath("symlink_link.txt");
        
        try {
            if (sys.FileSystem.exists(targetPath)) sys.FileSystem.deleteFile(targetPath);
            if (sys.FileSystem.exists(linkPath)) sys.FileSystem.deleteFile(linkPath);
        } catch(e:Dynamic) {}

        sys.io.File.saveContent(targetPath, "Symlink target content");
        
        if (digigun.sys.fs.Symlink.create(targetPath, linkPath)) {
            trace("  Symlink created.");
            var readPath = digigun.sys.fs.Symlink.read(linkPath);
            trace('  Read target path: $readPath');
            
            if (readPath != null && (readPath == targetPath || readPath.indexOf("symlink_target.txt") != -1)) {
                trace("  Symlink resolution: SUCCESS");
            } else {
                trace('  Symlink resolution: FAILED (Expected $targetPath, got $readPath)');
            }
        } else {
            trace("  Symlink creation: FAILED (Check permissions/Developer Mode)");
        }

        try {
            if (sys.FileSystem.exists(linkPath)) sys.FileSystem.deleteFile(linkPath);
            if (sys.FileSystem.exists(targetPath)) sys.FileSystem.deleteFile(targetPath);
        } catch(e:Dynamic) {}
    }

    static function testFileDiagnostics() {
        trace("--- Testing Advanced File Diagnostics (Phase: Stats/Permissions) ---");
        var testPath = getTestPath("diag_test.dat");
        var content = "Diagnostic data for 64-bit size test.";
        sys.io.File.saveContent(testPath, content);

        var stats = FileDiagnostics.stat(testPath);
        if (stats != null) {
            trace('  File Size: ${stats.size} bytes');
            trace('  File Type: ${stats.type.toString()}');
            trace('  Permissions Mask: 0x${StringTools.hex(stats.mode, 4)}');
            trace('  Modified Time: ${Date.fromTime(stats.mtime * 1000).toString()}');

            if (stats.size == content.length) trace("  64-bit Size Verification: SUCCESS");
            if (stats.type == RegularFile) trace("  File Type Verification: SUCCESS");

            // Test chmod (if POSIX)
            if (Sys.systemName() != "Windows") {
                trace("  Attempting chmod 0444 (read-only)...");
                if (FileDiagnostics.chmod(testPath, FilePermission.OWNER_READ | FilePermission.GROUP_READ | FilePermission.OTHERS_READ)) {
                    var newStats = FileDiagnostics.stat(testPath);
                    trace('    New permissions: 0x${StringTools.hex(newStats.mode, 4)}');
                }
            }
        } else {
            trace("  FAILED to retrieve file statistics.");
        }

        // Test Directory Type
        var dirType = FileDiagnostics.getType(getTestPath(""));
        trace('  Directory Type check: ${dirType.toString()}');
        if (dirType == Directory) trace("  Directory Type Verification: SUCCESS");

        try { if (sys.FileSystem.exists(testPath)) sys.FileSystem.deleteFile(testPath); } catch(e:Dynamic) {}
    }

    static function testVolume() {
        trace("--- Testing Volume Metadata ---");
        var path = Sys.systemName() == "Windows" ? "C:\\" : "/";
        var info = Volume.getInfo(path);
        if (info != null) {
            trace('  Path: $path');
            trace('  FS Type: ${info.fileSystem}');
            trace('  Label: ${info.name}');
            trace('  UUID: ${info.uuid}');
            trace('  Total Space: ${info.totalSpace / (1024 * 1024 * 1024)} GB');
            trace('  Free Space: ${info.freeSpace / (1024 * 1024 * 1024)} GB');
            trace("  Volume Info retrieval: SUCCESS");
        } else {
            trace("  FAILED to retrieve volume metadata.");
        }
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
        trace("--- Testing Process (Unified) ---");
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

    static function testMemoryProtection() {
        trace("--- Testing Direct Memory Protection (mprotect) ---");
        var pageSize = MemoryProtection.getPageSize();
        trace('  System Page Size: $pageSize bytes');

        // Allocate a buffer
        var mem = new digigun.sys.io.NativeBuffer(pageSize);

        // 1. Set READ_WRITE (default usually)
        trace("  Applying READ_WRITE...");
        if (MemoryProtection.protect(mem, READ_WRITE)) {
            trace("  READ_WRITE applied: SUCCESS");
        } else {
            trace("  READ_WRITE applied: FAILED");
        }

        // 2. Set READ only
        trace("  Applying READ_ONLY...");
        if (MemoryProtection.protect(mem, READ)) {
            trace("  READ_ONLY applied: SUCCESS");
        } else {
            trace("  READ_ONLY applied: FAILED");
        }

        // Re-enable WRITE so we can safely free or use later
        MemoryProtection.protect(mem, READ_WRITE);
        mem.free();
    }

    static function testSerialPort() {
        trace("--- Testing Serial Port / UART ---");
        var portName = switch (Sys.systemName()) {
            case "Windows": "COM99";
            default: "/dev/tty.digigun_dummy";
        };

        trace('  Attempting to open non-existent port: $portName');
        var serial = SerialPort.open(portName, 115200);
        if (serial != null) {
            trace("  Port opened (Unexpected!).");
            serial.close();
        } else {
            trace("  Port opening failed as expected. Structural check: SUCCESS");
        }
    }

    static function testRawSocket() {
        trace("--- Testing Raw Network Sockets (Packet Sniffing) ---");
        var iface = switch (Sys.systemName()) {
            case "Windows": "127.0.0.1";
            default: "lo0";
        };

        trace('  Attempting to open raw sniffer on interface: $iface');
        var sniffer = RawSocket.create(iface, true);
        if (sniffer != null) {
            trace("  Raw sniffer opened successfully.");
            var buf = new digigun.sys.io.NativeBuffer(65536);
            
            trace("  Waiting for a packet...");
            var bytes = sniffer.readPacket(buf);
            if (bytes > 0) {
                trace('  Captured raw packet of $bytes bytes.');
            } else {
                trace("  No packets captured during timeout.");
            }
            
            buf.free();
            sniffer.close();
            trace("  Raw sniffer closed.");
        } else {
            trace("  Skipping RawSocket test: Requires Root/Admin privileges.");
        }
    }

    static function testCrashHandler() {
        trace("--- Testing Resilient Diagnostics (Phase 5) ---");
        var reportPath = getTestPath("crash_report.txt");
        trace('  Setting up crash handler with report path: $reportPath');
        
        if (digigun.sys.rt.RtControl.setupCrashHandler(reportPath)) {
            trace("  Crash handler installed. Triggering intentional native crash...");
            Sys.sleep(0.5);
            // Null pointer dereference
            untyped __cpp__("*(int*)0 = 42;");
        } else {
            trace("  FAILED to setup crash handler.");
        }
    }
}

class FloatExt {
    public static function format(f:Float, precision:Int):String {
        return Std.string(Math.round(f * Math.pow(10, precision)) / Math.pow(10, precision));
    }
}
