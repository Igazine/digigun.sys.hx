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
import sys.FileSystem;

class Main {
    static function main() {
        // Check for Windows fork emulation flag
        var args = Sys.args();
        if (args.indexOf("--digigun-child-fork") != -1) {
            trace('[FORK-CHILD] Process ID: ${Process.getId()}');
            Sys.sleep(1);
            Sys.exit(0);
        }

        trace("digigun.sys.hx Functional Test Suite");
        trace('Current PID: ${Process.getId()}');

        // Basic Info & Time
        testSystemInfo();
        testTime();

        // Identity
        testAuth();

        // Security
        testRandom();

        // Process & Control
        testProcess();
        testProcessTree();
        testProcControl();
        testFork();
        testSignal();
        testRtControl();

        // File System & I/O
        testFileLock();
        testMemoryMap();
        testWatcher();
        testExtendedAttributes();
        testDirectIO();
        
        // IPC & Sockets
        testFifo();
        testUnixDomainSocket();
        testNonBlockingFifo();
        testReadAll();
        testSelector();
        testSharedMemory();
        testSemaphore();

        // Network
        testNetwork();
        testSendFile();

        if (args.indexOf("--tui") != -1) {
            testConsoleTUI();
        } else {
            trace("Skipping Console TUI test (use --tui to enable).");
        }

        trace("All architecture and implementation checks complete.");
    }

    static function testAuth() {
        trace("--- Testing Auth (Identity) ---");
        var current = Auth.getCurrentUser();
        if (current != null) {
            trace('  Current User: ${current.username} (UID: ${current.uid}, GID: ${current.gid})');
            trace('  Real Name: ${current.realname}');
            trace('  Home Dir: ${current.homeDir}');

            var byName = Auth.getUserByName(current.username);
            if (byName != null && byName.username == current.username) {
                trace("  User lookup by name: SUCCESS");
            } else {
                trace("  User lookup by name: FAILED");
            }
        } else {
            trace("  FAILED to get current user.");
        }

        var groups = Auth.getGroups();
        if (groups.length > 0) {
            trace('  Successfully listed ${groups.length} system groups.');
            trace('  First group: ${groups[0].name} (GID: ${groups[0].gid})');
        } else {
            #if windows
            trace("  Group listing: SKIPPED (Not implemented on Windows)");
            #else
            trace("  Group listing: FAILED (No groups found on POSIX)");
            #end
        }
    }

    static function testRandom() {
        trace("--- Testing Secure Random & UUID ---");
        var bytes = Random.getBytes(16);
        if (bytes != null && bytes.length == 16) {
            trace('  Successfully generated 16 secure bytes: ${bytes.toHex()}');
        } else {
            trace("  FAILED to generate secure bytes.");
        }

        var uuid1 = Random.uuid();
        var uuid2 = Random.uuid();
        trace('  UUID 1: ${uuid1}');
        trace('  UUID 2: ${uuid2}');

        if (uuid1 != null && uuid1.length == 36 && uuid1 != uuid2) {
            // Basic regex for UUID v4: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
            var r = ~/^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/;
            if (r.match(uuid1)) {
                trace("  UUID verification: SUCCESS (Format and Version 4 bits validated)");
            } else {
                trace("  UUID verification: FAILED (Regex mismatch)");
            }
        } else {
            trace("  UUID verification: FAILED (Length or collision)");
        }
    }

    static function testReadAll() {
        trace("--- Testing readAll() ---");
        var socketPath = "haxe_readall_test_sock";
        try { if (FileSystem.exists(socketPath)) FileSystem.deleteFile(socketPath); } catch(e:Dynamic) {}
        
        var server = UnixDomainSocket.create();
        server.bind(socketPath);
        server.listen(1);
        
        var client = UnixDomainSocket.create();
        
        // This is a bit tricky to test synchronously without threads, 
        // but we can try non-blocking readAll on an empty socket.
        client.connect(socketPath);
        client.setBlocking(false);
        
        var data = client.readAll();
        trace('  Empty readAll length: ${data.length}');
        
        // In a real test we'd send data from another process/thread.
        // For now, we just verify it doesn't crash and handles "no data" correctly.
        
        server.close();
        client.close();
        try { if (FileSystem.exists(socketPath)) FileSystem.deleteFile(socketPath); } catch(e:Dynamic) {}
    }

    static function testSignal() {
        trace("--- Testing Signal Handling ---");
        var os = Sys.systemName();
        var sigToTest:Int;
        
        if (os == "Windows") {
            trace("  Detected platform: Windows (via Sys.systemName)");
            sigToTest = Signal.INT;
        } else {
            trace('  Detected platform: ${os} (via Sys.systemName)');
            sigToTest = Signal.USR1;
        }
        
        var received = false;
        Signal.trap(sigToTest, (s) -> {
            trace('  [SIGNAL] Received signal: ${s}');
            received = true;
        });
        
        trace('  Raising signal ${sigToTest}... (Application may exit if signal is not caught)');
        Signal.raise(sigToTest);
        
        // Wait a tiny bit as signals are asynchronous
        Sys.sleep(0.1);
        
        if (received) trace("Signal handling verification: SUCCESS");
        else trace("Signal handling verification: FAILED (Timeout)");
    }

    static function testRtControl() {
        trace("--- Testing Real-time Performance (mlockall) ---");
        var isRoot = Process.isRoot();
        trace('  Attempting mlockall (Is Root: ${isRoot})...');
        var success = RtControl.mlockall(true, true);
        trace('  mlockall success: ${success}');
        if (success) {
            RtControl.munlockall();
            trace("  munlockall success: true");
        }
    }

    static function testNonBlockingFifo() {
        trace("--- Testing Non-blocking I/O (Socket) ---");
        var socketPath = "haxe_socket_nb_test";
        try { if (FileSystem.exists(socketPath)) FileSystem.deleteFile(socketPath); } catch(e:Dynamic) {}
        
        var server = UnixDomainSocket.create();
        server.bind(socketPath);
        server.listen(1);
        server.setBlocking(false);
        
        try {
            var client = server.accept();
            trace("Error: Accept should have blocked.");
        } catch (e:haxe.io.Error) {
            if (e == Blocked) trace("Success: Accept blocked as expected.");
            else trace("Error: Unexpected exception: " + e);
        }
        
        server.close();
        try { if (FileSystem.exists(socketPath)) FileSystem.deleteFile(socketPath); } catch(e:Dynamic) {}
    }

    static function testSelector() {
        trace("--- Testing Selector ---");
        var socketPath = "haxe_sel_test_sock";
        try { if (FileSystem.exists(socketPath)) FileSystem.deleteFile(socketPath); } catch(e:Dynamic) {}
        
        var s1 = UnixDomainSocket.create();
        s1.bind(socketPath);
        s1.listen(1);
        
        trace("Selecting with 0.1s timeout (should be empty)...");
        var ready = digigun.sys.fifo.Selector.select([s1], null, 0.1);
        trace('Ready for read: ${ready.read.length}');
        
        s1.close();
        try { if (FileSystem.exists(socketPath)) FileSystem.deleteFile(socketPath); } catch(e:Dynamic) {}
    }

    static function testFork() {
        trace("--- Testing Process.fork() ---");
        var pid = Process.fork();
        if (pid == -1) {
            trace("Fork failed.");
        } else if (pid == 0) {
            // This path only executes on POSIX systems
            trace('[FORK-CHILD-POSIX] I am the child. PID: ' + Process.getId());
            Sys.exit(0);
        } else {
            trace('[FORK-PARENT] Spawned child PID: ${pid}');
        }
    }

    static function testConsoleTUI() {
        trace("--- Testing Console TUI Primitives ---");
        
        var size = Console.getTerminalSize();
        if (size != null) {
            trace('Terminal Size: ${size.width}x${size.height}');
            
            trace("Drawing a colored box using writeBlock...");
            var w = 10; var h = 3;
            var chars = []; var attrs = [];
            for (i in 0...(w * h)) {
                chars.push(35); // '#'
                // Windows uses bitmask, POSIX uses 16-color ANSI code 
                // We'll use 31 (Red) which is common for testing.
                #if windows
                attrs.push(0x0004 | 0x0008); // RED | INTENSITY
                #else
                attrs.push(31); 
                #end
            }
            Console.writeBlock(2, 2, w, h, chars, attrs);
            Sys.sleep(1);
        }
        
        trace("Enabling Alternate Buffer...");
        Console.useAlternateBuffer(true);
        Sys.sleep(1);
        Console.useAlternateBuffer(false);
        trace("Back to main buffer.");
    }

    static function testTime() {
        trace("--- Testing Monotonic Time ---");
        var start = Time.stamp();
        var startNano = Time.nanoStamp();
        var sum = 0.0;
        for (i in 0...1000000) {
            sum += Math.sin(i);
        }
        var end = Time.stamp();
        var endNano = Time.nanoStamp();
        var elapsed = end - start;
        var elapsedNano = haxe.Int64.sub(endNano, startNano);
        trace('Elapsed time (seconds): ${elapsed}s');
        trace('Elapsed time (nanoseconds): ${Std.string(elapsedNano)}ns');
        if (elapsed > 0) trace("Time verification: SUCCESS");
    }

    static function testMemoryMap() {
        trace("--- Testing Memory Mapped Files ---");
        var mmapFile = "test_mmap.dat";
        var testStr = "Memory Mapping is fast!";
        var testData = haxe.io.Bytes.ofString(testStr);
        var mmap = MemoryMap.open(mmapFile, 1024, true);
        if (mmap != null) {
            trace("Memory Map opened successfully.");
            mmap.write(0, testData); mmap.flush();
            var readData = mmap.read(0, testData.length);
            var readStr = readData.toString();
            trace('Read back: "${readStr}"');
            if (readStr == testStr) trace("Memory Map verification: SUCCESS");
            mmap.close();
            var fileContent = sys.io.File.getContent(mmapFile);
            if (fileContent.indexOf(testStr) == 0) trace("File consistency verification: SUCCESS");
        } else trace("Failed to open Memory Map.");
        try { FileSystem.deleteFile(mmapFile); } catch(e:Dynamic) {}
    }

    static function testProcControl() {
        trace("--- Testing Process Control (Affinity/Priority) ---");
        var affinity = ProcControl.getAffinity();
        trace('Current CPU Affinity Mask: ${affinity}');
        
        trace("Setting priority to BelowNormal (should succeed)...");
        var success = ProcControl.setPriority(BelowNormal);
        trace('  BelowNormal Success: ${success}');
        
        trace("Setting priority to AboveNormal (requires root on POSIX)...");
        var successAbove = ProcControl.setPriority(AboveNormal);
        var isRoot = Process.isRoot();
        trace('  AboveNormal Success: ${successAbove} (Is Root: ${isRoot})');
        
        ProcControl.setPriority(Normal);
    }

    static function testSemaphore() {
        trace("--- Testing Named Semaphore ---");
        var semName = "haxe_sem_test";
        var sem = new NamedSemaphore();
        sem.unlink();
        if (sem.open(semName, 1)) {
            trace("Semaphore opened successfully.");
            if (sem.wait()) {
                trace("Acquired.");
                if (!sem.tryWait()) trace("Success: Busy as expected.");
                if (sem.post()) trace("Released.");
                if (sem.tryWait()) { trace("Success: Re-acquired."); sem.post(); }
            }
            sem.close();
            sem.unlink();
        } else trace("Failed to open semaphore.");
    }

    static function testFileLock() {
        trace("--- Testing File Locking ---");
        var lockFile = "test_lock.dat";
        var lockId = FileLock.lock(lockFile, true, false);
        if (lockId != -1) {
            trace('Lock acquired (ID: ${lockId}).');
            var secondLockId = FileLock.lock(lockFile, true, false);
            if (secondLockId == -1) trace("Second lock failed as expected.");
            FileLock.unlock(lockId);
            trace("Lock released.");
            var thirdLockId = FileLock.lock(lockFile, true, false);
            if (thirdLockId != -1) { trace("Re-acquisition success."); FileLock.unlock(thirdLockId); }
        } else trace("Failed to acquire initial lock.");
        try { if (FileSystem.exists(lockFile)) FileSystem.deleteFile(lockFile); } catch(e:Dynamic) {}
    }

    static function testWatcher() {
        trace("--- Testing File System Watcher ---");
        var watchPath = "test_watch_dir";
        if (!FileSystem.exists(watchPath)) FileSystem.createDirectory(watchPath);
        Watcher.watch(watchPath, (event) -> {
            trace('FS EVENT: Type=${event.type}, Path=${event.path}, isDir=${event.isDir}');
        });
        var testFile = watchPath + "/test.txt";
        sys.io.File.saveContent(testFile, "Hello Watcher");
        Sys.sleep(1);
        FileSystem.deleteFile(testFile);
        Sys.sleep(1);
        Watcher.stopAll();
        try { if (FileSystem.exists(watchPath)) FileSystem.deleteDirectory(watchPath); } catch(e:Dynamic) {}
    }

    static function testSystemInfo() {
        trace("--- Testing System Info ---");
        var mem = SystemInfo.getMemoryInfo();
        trace('RAM: Total=${formatBytes(mem.total)}, Free=${formatBytes(mem.free)}, Used=${formatBytes(mem.used)}');
        var cpu = SystemInfo.getCpuUsage();
        trace('CPU Usage: ${cpu}%');
        var disk = SystemInfo.getDiskInfo();
        trace('Disk (/): Total=${formatBytes(disk.total)}, Free=${formatBytes(disk.free)}, Available=${formatBytes(disk.available)}');
    }

    static function formatBytes(bytes:Float):String {
        if (bytes == -1) return "N/A";
        var units = ["B", "KB", "MB", "GB", "TB"];
        var unitIndex = 0;
        while (bytes >= 1024 && unitIndex < units.length - 1) {
            bytes /= 1024;
            unitIndex++;
        }
        return Math.round(bytes * 100) / 100 + " " + units[unitIndex];
    }

    static function testSharedMemory() {
        trace("--- Testing Shared Memory ---");
        var shmName = "haxe_shm_test";
        var shm = new SharedMemory();
        shm.unlink();
        if (shm.open(shmName, 1024, true)) {
            trace("Shared Memory segment opened successfully.");
            var testStr = "Hello from Shared Memory!";
            var testData = haxe.io.Bytes.ofString(testStr);
            shm.write(0, testData);
            var readData = shm.read(0, testData.length);
            var readStr = readData.toString();
            trace('Read back: "${readStr}"');
            if (readStr == testStr) trace("Shared Memory verification: SUCCESS");
            shm.close();
            shm.unlink();
        } else trace("Failed to open Shared Memory segment.");
    }

    static function testProcess() {
        trace("--- Testing Process Functions ---");
        trace('Is Root/Admin: ${Process.isRoot()}');
        var currentLimit = Process.getFileResourceLimit();
        trace('Current File Limit: ${currentLimit}');
        if (currentLimit != -1) {
            var newLimit = Process.setFileResourceLimit(2048);
            trace('New File Limit set to: ${newLimit}');
        }
        trace("Listing Processes (Top 5)...");
        var procs = Process.listProcesses();
        trace('Total processes found: ${procs.length}');
        var count = 0;
        for (p in procs) {
            trace('  PID: ${p.pid} | PPID: ${p.ppid} | Name: ${p.name} | RSS: ${formatBytes(p.memoryRss)} | VIRT: ${formatBytes(p.memoryVirtual)}');
            if (++count >= 5) break;
        }
    }

    static function testProcessTree() {
        trace("--- Testing Process Tree (Hierarchical) ---");
        var myPid = Process.getId();
        trace('Retrieving tree for current process (PID: ${myPid})...');
        var tree = Process.getProcessTree(myPid);
        if (tree.length > 0) {
            printTree(tree[0], 0);
        } else {
            trace("  Could not find current process in tree.");
        }
    }

    static function printTree(p:digigun.sys.process.Process.ProcessInfo, indent:Int) {
        var prefix = "";
        for (i in 0...indent) prefix += "  ";
        trace('${prefix}PID: ${p.pid} | Name: ${p.name}');
        for (child in p.children) {
            printTree(child, indent + 1);
        }
    }

    static function testNetwork() {
        trace("--- Testing Network Functions ---");
        trace("Retrieving ARP Table...");
        var arpEntries = NetworkControl.getArpTable();
        trace('Found ${arpEntries.length} ARP entries.');
        for (i in 0...Std.int(Math.min(arpEntries.length, 5))) {
            var entry = arpEntries[i];
            trace('  IP: ${entry.ip} | MAC: ${entry.mac} | Interface: ${entry.interfaceName}');
        }
        trace("Resolving 'google.com'...");
        var ips = Network.getHostInfo("google.com");
        if (ips.length > 0) trace("Found IPs: " + ips.join(", "));
        trace("Listing Network Interfaces...");
        var interfaces = Network.getNetworkInterfaces();
        for (ni in interfaces) {
            trace('Interface: ${ni.name} | IPv4: ${ni.ipv4} | MAC: ${ni.mac} | Flags: ${ni.flags}');
        }

        trace("--- Testing Socket Options ---");
        var sock = new sys.net.Socket();
        try {
            // TCP_NODELAY is a common option to toggle
            var success = NetworkControl.setSocketOptionBool(sock, SocketOption.IPPROTO_TCP, SocketOption.TCP_NODELAY, true);
            trace('  Set TCP_NODELAY: ${success ? "SUCCESS" : "FAILED (expected if socket not connected)"}');
        } catch(e:Dynamic) {
            trace("  Set TCP_NODELAY error: " + e);
        }
        sock.close();

        trace("--- Testing Ping (Single) ---");
        var rtt = Network.ping("8.8.8.8");
        if (rtt >= 0) trace('  Ping successful: RTT = ${rtt}ms');
        
        trace("--- Testing Mass Ping (PingSession) ---");
        var session = new PingSession();
        var targets = ["8.8.8.8", "1.1.1.1", "127.0.0.1", "google.com"];
        for (t in targets) {
            var seq = session.send(t);
            trace('  Sent ping to ${t} (seq: ${seq})');
        }

        // Wait up to 1 second for replies
        var totalWait = 0.0;
        var receivedCount = 0;
        while (totalWait < 1.0 && receivedCount < targets.length) {
            var replies = session.collect();
            for (r in replies) {
                trace('  Reply from ${r.host}: RTT = ${Math.round(r.rtt * 100) / 100}ms (seq: ${r.seq})');
                receivedCount++;
            }
            Sys.sleep(0.1);
            totalWait += 0.1;
        }
        trace('  Mass ping complete. Received ${receivedCount}/${targets.length} replies.');
        session.close();
    }

    static function testSendFile() {
        trace("--- Testing Zero-copy I/O (sendFile) ---");
        var testFile = "test_sendfile.dat";
        var testData = "Zero-copy transfer is efficient!";
        sys.io.File.saveContent(testFile, testData);
        
        var socketPath = "haxe_sendfile_sock";
        try { if (FileSystem.exists(socketPath)) FileSystem.deleteFile(socketPath); } catch(e:Dynamic) {}
        
        var server = UnixDomainSocket.create();
        server.bind(socketPath);
        server.listen(1);
        
        var client = UnixDomainSocket.create();
        // Use a background task or just assume non-blocking connect for this simple test
        // On macOS connect to local socket is usually fast.
        
        // Actually, let's use a simpler verification: 
        // Just verify the function call doesn't crash and returns a value.
        // In a real scenario we'd have a receiver.
        
        trace("  Note: sendFile requires a connected socket. Testing signature only.");
        // Since we don't have a full async handshake here, we'll just check if it handles error gracefully.
        var sent = digigun.sys.io.IO.sendFile(client, testFile, 0, testData.length);
        trace('  sent result: ${sent} (expected -1 or length if connected)');
        
        server.close();
        client.close();
        try { if (FileSystem.exists(testFile)) FileSystem.deleteFile(testFile); } catch(e:Dynamic) {}
        try { if (FileSystem.exists(socketPath)) FileSystem.deleteFile(socketPath); } catch(e:Dynamic) {}
    }

    static function testDirectIO() {
        trace("--- Testing Direct I/O (F_NOCACHE/O_DIRECT) ---");
        #if (windows || win32)
        trace("  Note: Toggling Direct I/O on existing handles is not supported on Windows.");
        #end
        var testFile = "test_directio.dat";
        var h = digigun.sys.io.IO.openFile(testFile, true);
        
        var success = digigun.sys.io.IO.setDirectIO(h, true);
        trace('  setDirectIO(true) success: ${success}');
        
        digigun.sys.io.IO.closeHandle(h);
        
        if (FileSystem.exists(testFile)) FileSystem.deleteFile(testFile);
    }

    static function testFifo() {
        var fifoPath = "/tmp/haxe_fifo_test";
        try { if (FileSystem.exists(fifoPath)) FileSystem.deleteFile(fifoPath); } catch(e:Dynamic) {}
        var fifo = new Fifo();
        trace("Testing FIFO creation at: " + fifoPath);
        if (fifo.create(fifoPath)) trace("FIFO created successfully.");
        fifo.close();
    }

    static function testUnixDomainSocket() {
        var socketPath = "haxe_socket_test";
        try { if (FileSystem.exists(socketPath)) FileSystem.deleteFile(socketPath); } catch(e:Dynamic) {}
        var server = UnixDomainSocket.create();
        trace("Testing Unix Domain Socket bind at: " + socketPath);
        if (server.bind(socketPath)) {
            trace("Socket bound successfully.");
            if (server.listen()) trace("Socket listening successfully.");
        }
        server.close();
    }

    static function testExtendedAttributes() {
        trace("--- Testing Extended Attributes (xattr / ADS) ---");
        var testFile = "test_xattr.dat";
        sys.io.File.saveContent(testFile, "Extended Attributes test content");

        var attrName = "test_attr";
        #if linux
        attrName = "user.test_attr"; // Linux requires 'user.' prefix for user attributes
        #end
        
        var testValue = haxe.io.Bytes.ofString("Hello from Extended Attributes!");

        trace('  Setting attribute "${attrName}"...');
        if (ExtendedAttributes.set(testFile, attrName, testValue)) {
            trace("  Attribute set: SUCCESS");
            
            trace("  Getting attribute...");
            var retrievedValue = ExtendedAttributes.get(testFile, attrName);
            if (retrievedValue != null && retrievedValue.toString() == testValue.toString()) {
                trace('  Attribute get: SUCCESS (Value: "${retrievedValue.toString()}")');
            } else {
                trace("  Attribute get: FAILED");
            }

            trace("  Listing attributes...");
            var attrs = ExtendedAttributes.list(testFile);
            trace('  Found ${attrs.length} attributes: ' + attrs.join(", "));
            if (attrs.indexOf(attrName) != -1) {
                trace("  Attribute listing: SUCCESS");
            } else {
                trace("  Attribute listing: FAILED");
            }

            trace("  Removing attribute...");
            if (ExtendedAttributes.remove(testFile, attrName)) {
                trace("  Attribute removal: SUCCESS");
                if (ExtendedAttributes.get(testFile, attrName) == null) {
                    trace("  Attribute removal verification: SUCCESS");
                } else {
                    trace("  Attribute removal verification: FAILED (Still exists)");
                }
            } else {
                trace("  Attribute removal: FAILED");
            }
        } else {
            #if linux
            trace("  Attribute set: FAILED (Expected on some Docker filesystems like overlayfs)");
            trace("  Note: Linux xattr requires ext4/xfs/btrfs with user_xattr support.");
            #else
            trace("  Attribute set: FAILED");
            #end
        }

        try { if (sys.FileSystem.exists(testFile)) sys.FileSystem.deleteFile(testFile); } catch(e:Dynamic) {}
    }
}
