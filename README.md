# digigun.sys.hx

Zero-dependency system extension library for Haxe (CPP target) to extend Haxe with system functions that are not available in the standard library.

## Table of Contents

- [Features](#features)
- [Status Legend](#status-legend)
- [Roadmap](#roadmap)
- [Installation](#installation)
- [Target Operating Systems](#target-operating-systems)
- [Contributions & Bug Reports](#contributions--bug-reports)
- [Usage Examples](#usage-examples)
- [Safety & Security Considerations](#safety--security-considerations)
- [License](#license)

## Features

- **FIFO (Named Pipes)**: Create and communicate through local named pipes.
- **Unix Domain Sockets**: High-performance local bidirectional Inter-Process Communication (IPC).
- **Network Functions**: DNS resolution, interface listing, unprivileged ping, `setsockopt` wrappers, ARP table extraction, and high-precision mass-ping (`PingSession`).
- **Shared Memory**: Cross-process shared memory segments.
- **System Info**: RAM, CPU, and Disk diagnostics.
- **File System**: Native recursive watcher, advisory locking, and memory mapping.
- **I/O Optimization**: Zero-copy `sendFile` and `DirectIO` support.
- **Process Control**: Affinity, priority, and resource limits.
- **Signal Handling**: Native signal trapping for POSIX and Windows.
- **TUI/Console**: Low-level terminal control and block writing.
- **Secure Random**: CSPRNG and UUID v4 support.
- **Identity & Credentials**: Access to system user and group metadata.
- **System Service Integration**: Integration with `systemd` (sd_notify) and Windows SCM.
- **Dynamic Symbol Loading**: Runtime loading of shared libraries (.dylib, .so, .dll) and symbol resolution.
- **FFI Infrastructure**: Smart `@:build` macros for automated proxy generation with `String`/`Bool` auto-conversion and automated struct layout mapping.
- **Native Event Loops**: Completion-based (Proactor) engine for high-performance async I/O (kqueue, epoll, IOCP).
- **Async File I/O**: Non-blocking file operations with native buffer management to minimize GC overhead.
- **Advanced Native Buffers**: Optimized `RingBuffer`, `BipBuffer` (contiguous), and `ChunkedBuffer` (linked) for high-speed streaming.
- **Resilient Diagnostics**: Native crash interception for Segfaults and Access Violations with minidump generation.
- **Symlink Management**: Cross-platform symbolic link creation and target resolution.

## Status Legend

- ✅ **Implemented** - Feature is complete and working
- 🚧 **In Progress** - Feature is being developed
- ⏳ **Planned** - Feature is scheduled for future development
- 💡 **Requested** - Feature has been requested but not scheduled
- ❌ **Not Planned** - Feature will not be implemented

## Roadmap

### FIFO / Unix Domain Socket (`digigun.sys.fifo`) ✅
- [x] FIFO Implementation (Unix/macOS/Windows) ✅
- [x] Unix Domain Socket Implementation (Unix/macOS/Windows) ✅
- [x] Cross-platform support via Named Pipes (Windows) and AF_UNIX (POSIX) ✅
- [x] Asynchronous/Non-blocking I/O support and Selector mechanism ✅

### Network Functions (`digigun.sys.network`) ✅
- [x] `getHostInfo()` - DNS resolution (getaddrinfo) ✅
- [x] `getNetworkInterfaces()` - Available interfaces (getifaddrs) ✅
- [x] `ping()` - ICMP implementation (unprivileged) ✅
- [x] `getArpTable()` - Retrieve system ARP table ✅
- [x] `bindToInterface()` - Bind socket to specific NIC ✅
- [x] `setSocketOption()` - High-level `setsockopt` wrappers ✅
- [x] `PingSession` - High-performance mass ping with Kernel Timestamping (POSIX) ✅

### Shared Memory (`digigun.sys.shm`) ✅
- [x] Create and open shared segments (Unix/macOS/Windows) ✅
- [x] Secure private mode support ✅
- [x] Direct memory read/write ✅

### System Info (`digigun.sys.info`) ✅
- [x] RAM statistics (total, free, used) ✅
- [x] CPU usage percentage ✅
- [x] Disk space stats ✅

### Process Functions (`digigun.sys.process`) ✅
- [x] `isRoot()` - Check for root/admin privileges ✅
- [x] `getFileResourceLimit()` - Get open file descriptor limit ✅
- [x] `setFileResourceLimit()` - Set open file descriptor limit ✅
- [x] `listProcesses()` - List running processes and their stats ✅
- [x] `getProcessTree()` - Hierarchical process listing ✅

### File System (`digigun.sys.fs`, `digigun.sys.io`) ✅
- [x] `Watcher` - Native event-driven recursive watching ✅
- [x] `FileLock` - Advisory file locking (exclusive/shared) ✅
- [x] `MemoryMap` - High-performance file memory mapping ✅
- [x] `sendFile()` - Efficient zero-copy file-to-socket transfer ✅
- [x] `setDirectIO()` - Bypass OS page cache (O_DIRECT / F_NOCACHE) ✅

### Real-time & Signal Handling (`digigun.sys.rt`, `digigun.sys.signal`) ✅
- [x] `mlockall()` - Lock process memory into RAM ✅
- [x] `trap()` - Native signal handling (SIGUSR1, SIGUSR2, etc.) ✅

### Secure Random & Entropy (`digigun.sys.random`) ✅
- [x] `getBytes()` - Cryptographically secure random bytes ✅
- [x] `uuid()` - Version 4 UUID generation ✅

### Identity & Credentials (`digigun.sys.auth`) ✅
- [x] `getCurrentUser()` - Information about the current user ✅
- [x] `getUser(id)` - Polymorphic user lookup by UID or username ✅
- [x] `getGroups()` - List system groups ✅

### Inter-Process Synchronization (`digigun.sys.sync`) ✅
- [x] `NamedSemaphore` - Cross-process named semaphores ✅

### Process Control (`digigun.sys.proc`) ✅
- [x] `setAffinity()` - Pin process to CPU cores ✅
- [x] `setPriority()` - Set process scheduling priority ✅

### High-Resolution Timing (`digigun.sys.time`) ✅
- [x] `stamp()` - Monotonic seconds ✅
- [x] `nanoStamp()` - Monotonic nanoseconds ✅

### Console & TUI (`digigun.sys.console`) ✅
- [x] `setRawMode()` - Toggle terminal raw mode ✅
- [x] `getTerminalSize()` - Get window dimensions ✅
- [x] `writeBlock()` - Fast character block writing ✅
- [x] `useAlternateBuffer()` - Switch to alternate screen ✅

### Extended File System Attributes (`digigun.sys.fs` expansion) ✅
- [x] `getXAttr()` - Get extended file attribute ✅
- [x] `setXAttr()` - Set extended file attribute ✅
- [x] `listXAttrs()` - List extended file attributes ✅

### System Service Integration (`digigun.sys.service`) ✅
- [x] `notify()` - systemd `sd_notify` support verified on Linux ✅
- [x] `run()` - Initial Windows SCM implementation ✅

### Dynamic Symbol Loading (`digigun.sys.dl`) ✅
- [x] `open()` - Load shared libraries (.so, .dylib, .dll) ✅
- [x] `getSymbol()` - Resolve function pointers at runtime ✅
- [x] **FFI Build Macro** - Automate `cpp.Callable` proxy generation via `@:build` ✅
- [x] **Smart Type Automation** - Automatic `String` and `Bool` conversion for FFI calls ✅
- [x] **FFI Struct Mapping** - Automated Haxe-to-C struct layout mapping via `@:struct` ✅

### Native Event Loops (`digigun.sys.io`) ✅
- [x] **Completion-based Architecture** - Native kqueue/epoll/IOCP support ✅
- [x] **Async read/write** - True non-blocking operations on sockets and pipes ✅
- [x] **Zero-overhead callbacks** - Direct native-to-Haxe callback dispatching ✅

### Async File I/O & Native Buffers (`digigun.sys.io` expansion) ✅
- [x] **Native Thread Pool (POSIX)** - Async file operations on macOS/Linux ✅
- [x] **Native Completion (Windows)** - Native IOCP support for regular files ✅
- [x] **NativeBuffer** - `malloc`-based memory management outside Haxe GC ✅
- [x] **AsyncFile API** - High-level completion-driven file API ✅

### Advanced Native Buffers (`digigun.sys.io` expansion) ✅
- [x] **RingBuffer** - Fixed-size native circular buffer ✅
- [x] **BipBuffer** - Bipartite contiguous memory buffer ✅
- [x] **ChunkedBuffer** - Dynamic linked native chunks ✅
- [x] **Haxe Integration** - Full `haxe.io.Input`/`Output` compatibility ✅

### Resilient Diagnostics (`digigun.sys.rt`) ✅
- [x] **Native Crash Handler** - Interception of SIGSEGV, SIGABRT, and Access Violations ✅
- [x] **Stack Trace Capture** - Text-based backtrace generation for all platforms ✅
- [x] **Minidump Generation** - Standard `.dmp` files for Windows ARM64/x64 ✅

### Symlink Management (`digigun.sys.fs`) ✅
- [x] **Cross-platform Creation** - `symlink` (POSIX) and `CreateSymbolicLink` (Windows) ✅
- [x] **Target Resolution** - `readlink` (POSIX) and `GetFinalPathNameByHandle` (Windows) ✅

### Future / Research ⏳
- [ ] `io_uring` - Kernel-level completion engine for Linux ⏳
- [ ] Phase 6: Fibers - Cooperative multitasking and stack-switching ⏳
- [ ] Phase 7: Advanced Synchronization - Native Futex and WaitOnAddress ⏳
- [ ] Tier 4: Hardware & Serial IPC - Unified UART and USB HID access ⏳

> **Note for Linux:** Extended attributes (xattr) require a supporting filesystem (e.g., ext4, xfs, btrfs) mounted with `user_xattr` support. Standard Docker `overlayfs` may not support the `user.` namespace used by this library.

> **Note for Linux (Network):** Unprivileged ping requires the `net.ipv4.ping_group_range` sysctl to be set. To enable for all users: `sudo sysctl -w net.ipv4.ping_group_range="0 2147483647"`.

## Installation

```bash
haxelib git digigun.sys.hx https://github.com/igazine/digigun.sys.hx
```

## FFI Library Generation

This library can be compiled into platform-specific shared libraries for use in other languages (Python, Rust, Node.js, etc.).

```bash
# Generate macOS .dylib
haxe test/build-lib-mac.hxml

# Generate Linux .so
haxe test/build-lib-linux.hxml

# Generate Windows .dll
haxe test/build-lib-win-arm.hxml
```

The resulting libraries use a stable C-ABI via the `DIGIGUN_API` export macro and a robust `Int64` handle architecture.

## Target Operating Systems

The library is primarily designed for desktop and server operating systems, including:
- **macOS** (Apple Silicon and Intel)
- **Linux** (including single-board computers like Raspberry Pi)
- **Windows** (x64 and ARM64)

### Platform Support & Development
All features are primarily developed and tested on **POSIX-compliant systems (macOS and Linux)**. While full compatibility with **Windows** is a core requirement and actively maintained, it serves as a secondary development platform.

Mobile operating systems (such as **Android and iOS**) might be partially supported depending on the specific module and the underlying kernel features available; however, full compatibility is not guaranteed.

## Contributions & Bug Reports

If you encounter any misbehavior, bugs, or platform-specific issues, please **open a GitHub issue**. Feedback and pull requests are welcome to help improve cross-platform stability.

## Usage Examples

### High-Performance Mass Ping
```haxe
import digigun.sys.network.PingSession;

var session = new PingSession();
var targets = ["8.8.8.8", "1.1.1.1", "google.com"];

for (host in targets) {
    session.send(host);
}

// Non-blocking collection of results
// POSIX targets use Kernel Timestamping to bypass application polling lag.
while (true) {
    var replies = session.collect();
    for (reply in replies) {
        trace('Reply from ${reply.host}: RTT = ${reply.rtt}ms');
    }
    if (/* all received or timeout */) break;
    Sys.sleep(0.01);
}
session.close();
```

### Socket Options
```haxe
import digigun.sys.network.NetworkControl;
import digigun.sys.network.SocketOption;

var sock = new sys.net.Socket();
// Disable Nagle's algorithm
NetworkControl.setSocketOptionBool(sock, SocketOption.IPPROTO_TCP, SocketOption.TCP_NODELAY, true);
```

### FIFO (Named Pipe)
```haxe
import digigun.sys.fifo.Fifo;

var fifo = new Fifo();
fifo.create("/tmp/my_pipe", 438); // 438 is 0666 octal
fifo.open("/tmp/my_pipe", true); // open for write
fifo.write(haxe.io.Bytes.ofString("Hello"), 5);
fifo.close();
```

### Unix Domain Socket
```haxe
import digigun.sys.fifo.UnixDomainSocket;
import digigun.sys.fifo.Selector;

var server = UnixDomainSocket.create();
server.bind("/tmp/my_socket");
server.listen(5);
server.setBlocking(false);

// Use Selector for non-blocking multiplexing
var ready = Selector.select([server], null, 1.0);
if (ready.read.length > 0) {
    var client = server.accept();
    // ...
}
```

### Shared Memory & Semaphores
```haxe
import digigun.sys.shm.SharedMemory;
import digigun.sys.sync.NamedSemaphore;

var sem = new NamedSemaphore();
if (sem.open("my_mutex", 1)) {
    sem.wait();
    
    var shm = new SharedMemory();
    if (shm.open("my_buffer", 1024, true)) {
        shm.write(0, haxe.io.Bytes.ofString("Shared data"));
        shm.close();
    }
    
    sem.post();
    sem.close();
}
```

### File System Watcher
```haxe
import digigun.sys.fs.Watcher;

Watcher.watch("./src", (event) -> {
    trace('Change detected: ${event.path} (${event.type})');
});

// To stop watching:
// Watcher.stopAll();
```

### Native Signal Handling
```haxe
import digigun.sys.signal.Signal;

Signal.trap(Signal.USR1, (signo) -> {
    trace('Received SIGUSR1 ($signo). Performing hot-reload...');
});
```

### Identity & Credentials
```haxe
import digigun.sys.auth.Auth;

var user = Auth.getCurrentUser();
if (user != null) {
    trace('Hello, ${user.realname} (${user.username})!');
    trace('Home: ${user.homeDir}');
}
```

### Automated FFI Macros
Define a proxy class with the `@:build` macro to automatically map native symbols from a dynamic library with smart type conversion.

```haxe
import digigun.sys.dl.FFI;

@:build(digigun.sys.dl.FFI.build())
class MyNativeLib {
    // Automatically resolved from the bound library
    @:native("process_get_id")
    public static function getPid():Int return 0;

    // String and Bool types are automatically converted to/from C-strings/int
    @:native("process_echo")
    public static function echo(input:String):String return null;
}
```

### FFI Struct Mapping
Automatically map Haxe classes to native C structs for passing complex data to/from dynamic libraries.

```haxe
import digigun.sys.dl.FFI;

@:build(digigun.sys.dl.FFI.struct())
class NativePoint {
    public var x:Int;
    public var y:Int;
}

@:build(digigun.sys.dl.FFI.build())
class MyStructLib {
    @:native("process_point")
    public static function process(p:NativePoint):Void {}
}

// ... usage ...
var p = new NativePoint();
p.x = 10;
p.y = 20;
MyStructLib.process(p); // Passes raw native pointer to C++
trace('Modified by C++: ${p.x}, ${p.y}');
```

### Async File I/O & Native Buffers
Perform non-blocking file operations with zero GC overhead by using native buffers.

```haxe
import digigun.sys.io.NativeLoop;
import digigun.sys.io.AsyncFile;
import digigun.sys.io.NativeBuffer;

var loop = new NativeLoop();
var file = AsyncFile.open(loop, "large_file.bin", false);

if (file != null) {
    // Read 1MB asynchronously
    file.read(1024 * 1024, (result, data) -> {
        if (result == 0) {
            trace('Read ${data.size} bytes into native memory.');
            // data.toBytes() to move to Haxe, or use getPointer() for native work
        }
        data.free(); // Manually free native memory
        file.close();
    });
}

// Keep polling the loop to process completions
while (true) {
    loop.poll(10); 
}
```

### Advanced Native Buffers
Use specialized buffers for zero-copy streaming and contiguous memory access.

```haxe
import digigun.sys.io.BipBuffer;

var buffer = new BipBuffer(1024 * 1024); // 1MB

// Contiguous write reservation
var res = buffer.reserve(4096);
if (res.ptr != null) {
    // res.ptr is a raw native pointer
    // populate res.ptr via native FFI or C++
    buffer.commit(4096);
}

// Contiguous read access
var info = buffer.getReadPtr();
if (info.ptr != null) {
    // Process info.len bytes from info.ptr
    buffer.decommit(info.len);
}

// Use with standard Haxe tools
var input = buffer.asInput();
var str = input.readString(10);
```

### Resilient Diagnostics (Minidumps)
Enable native crash interception to capture stack traces when a fatal error occurs.

```haxe
import digigun.sys.rt.RtControl;

// Setup a crash handler that writes to a specific file
// On Windows, this also generates a .dmp file at the same path
RtControl.setupCrashHandler("path/to/crash_report.txt");

// If a native segfault or access violation occurs, a report is generated
// containing the C++/Haxe stack trace.
```

### Symlink Management
Create and read symbolic links across platforms.

```haxe
import digigun.sys.fs.Symlink;

// Create a symlink
if (Symlink.create("target_file.txt", "my_link.txt")) {
    trace("Link created successfully");
}

// Read a symlink's target
var target = Symlink.read("my_link.txt");
trace('Points to: $target');
```

## Safety & Security Considerations

Users of this library should be aware of the following security implications:

- **Elevated Privileges:** Several system-level functions (e.g., setting CPU affinity, adjusting process priorities, or locking memory into RAM) may require root/administrator privileges depending on the operating system and security policies.
- **Inter-Process Communication (IPC):** Features such as Shared Memory, Named Semaphores, and Unix Domain Sockets allow data to cross process boundaries. These must be implemented with care to prevent race conditions, unauthorized access, or memory corruption.
- **Input Validation:** When using system functions that interact with the filesystem or network, always validate inputs to avoid injection-style vulnerabilities or accidental system instability.

---

*This library, its architectural structure, test suite, and development environments were architected by a human. Google Gemini was used throughout the process for code generation and implementation under the human architect's complete control (AI-assisted development).*
