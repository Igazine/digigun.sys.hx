# digigun.sys.hx

System extension library for Haxe (CPP target) to extend Haxe with system functions that are not available in the standard library.

## Features

- **FIFO (Named Pipes)**: Create and communicate through local named pipes.
- **Unix Domain Sockets**: High-performance local bidirectional Inter-Process Communication (IPC).
- **Network Functions**: DNS resolution, interface listing, and unprivileged ping.
- **Shared Memory**: Cross-process shared memory segments.
- **System Info**: RAM, CPU, and Disk diagnostics.
- **File System**: Native recursive watcher, advisory locking, and memory mapping.
- **I/O Optimization**: Zero-copy `sendFile` and `DirectIO` support.
- **Process Control**: Affinity, priority, and resource limits.
- **Signal Handling**: Native signal trapping for POSIX and Windows.
- **TUI/Console**: Low-level terminal control and block writing.

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

### Network Control (`digigun.sys.network`) ✅
- [x] `getArpTable()` - Retrieve system ARP table ✅
- [x] `bindToInterface()` - Bind socket to specific NIC ✅

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

> **Note for Linux:** Unprivileged ping requires the `net.ipv4.ping_group_range` sysctl to be set. To enable for all users: `sudo sysctl -w net.ipv4.ping_group_range="0 2147483647"`.

## Installation

```bash
haxelib git digigun.sys.hx https://github.com/igazine/digigun.sys.hx
```

## Usage Examples

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

### System Diagnostics
```haxe
import digigun.sys.info.SystemInfo;

var mem = SystemInfo.getMemoryInfo();
trace('RAM Used: ${Math.round(mem.used / 1024 / 1024)} MB');

var cpu = SystemInfo.getCpuUsage();
trace('CPU Usage: ${cpu}%');
```

### Native Signal Handling
```haxe
import digigun.sys.signal.Signal;

Signal.trap(Signal.USR1, (signo) -> {
    trace('Received SIGUSR1 ($signo). Performing hot-reload...');
});
```

### Process Control
```haxe
import digigun.sys.proc.ProcControl;

// Pin process to first 4 cores
ProcControl.setAffinity(0xF); 

// Set high priority
ProcControl.setPriority(AboveNormal);
```

## License

MIT
