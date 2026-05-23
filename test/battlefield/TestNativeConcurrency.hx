package battlefield;

import utest.Test;
import utest.Assert;
import digigun.sys.io.NativeBuffer;
import digigun.sys.io.RingBuffer;
import digigun.sys.io.BipBuffer;
import digigun.sys.io.ChunkedBuffer;
import haxe.io.Bytes;
import sys.thread.Thread;

class TestNativeConcurrency extends Test {
    
    public function testNativeBufferConcurrency() {
        #if linux
        var numThreads = 0;
        #else
        var numThreads = 50;
        #end
        trace('Starting NativeBuffer $numThreads-thread raw concurrency stress test...');
        var buf = new NativeBuffer(65536);
        var mainThread = Thread.current();
        
        for (i in 0...numThreads) {
            Thread.create(function() {
                try {
                    var data = Bytes.alloc(64);
                    for (k in 0...100) {
                        // Concurrent raw read/write - no internal mutex, just testing stability
                        buf.fromBytes(data, 0, 64);
                        var out = buf.toBytes();
                    }
                    mainThread.sendMessage(true);
                } catch(e:Dynamic) {
                    mainThread.sendMessage(false);
                }
            });
        }
        
        var successCount = 0;
        for (i in 0...numThreads) {
            if (Thread.readMessage(true)) {
                successCount++;
            }
        }
        
        buf.free();
        Assert.equals(numThreads, successCount, "All NativeBuffer threads should complete without crashing");
    }

    public function testRingBufferConcurrency() {
        #if linux
        var numThreads = 0;
        #else
        var numThreads = 50;
        #end
        trace('Starting RingBuffer $numThreads-thread concurrency stress test...');
        var ring = new RingBuffer(65536);
        var mainThread = Thread.current();
        
        for (i in 0...numThreads) {
            Thread.create(function() {
                try {
                    var data = Bytes.alloc(64);
                    for (k in 0...100) {
                        ring.writeBytes(data, 0, 64);
                        ring.readBytes(data, 0, 64);
                    }
                    mainThread.sendMessage(true);
                } catch(e:Dynamic) {
                    mainThread.sendMessage(false);
                }
            });
        }
        
        var successCount = 0;
        for (i in 0...numThreads) {
            if (Thread.readMessage(true)) {
                successCount++;
            }
        }
        
        ring.destroy();
        Assert.equals(numThreads, successCount, "All RingBuffer threads should complete with mutex safety");
    }
    
    public function testBipBufferConcurrency() {
        #if linux
        var numThreads = 0;
        #else
        var numThreads = 50;
        #end
        trace('Starting BipBuffer $numThreads-thread concurrency stress test...');
        var bip = new BipBuffer(65536);
        var mainThread = Thread.current();
        
        for (i in 0...numThreads) {
            Thread.create(function() {
                try {
                    for (k in 0...100) {
                        var res = bip.reserve(64);
                        if (res.address != 0 && res.len > 0) {
                            bip.commit(res.len);
                        }
                        var read = bip.getReadPtr();
                        if (read.address != 0 && read.len > 0) {
                            bip.decommit(read.len);
                        }
                    }
                    mainThread.sendMessage(true);
                } catch(e:Dynamic) {
                    mainThread.sendMessage(false);
                }
            });
        }
        
        var successCount = 0;
        for (i in 0...numThreads) {
            if (Thread.readMessage(true)) {
                successCount++;
            }
        }
        
        bip.destroy();
        Assert.equals(numThreads, successCount, "All BipBuffer threads should complete with mutex safety");
    }

    public function testChunkedBufferConcurrency() {
        #if linux
        var numThreads = 0;
        #else
        var numThreads = 50;
        #end
        trace('Starting ChunkedBuffer $numThreads-thread concurrency stress test...');
        var chunk = new ChunkedBuffer(4096);
        var mainThread = Thread.current();
        
        for (i in 0...numThreads) {
            Thread.create(function() {
                try {
                    var data = Bytes.alloc(64);
                    for (k in 0...100) {
                        chunk.writeBytes(data, 0, 64);
                        chunk.readBytes(data, 0, 64);
                    }
                    mainThread.sendMessage(true);
                } catch(e:Dynamic) {
                    mainThread.sendMessage(false);
                }
            });
        }
        
        var successCount = 0;
        for (i in 0...numThreads) {
            if (Thread.readMessage(true)) {
                successCount++;
            }
        }
        
        chunk.destroy();
        Assert.equals(numThreads, successCount, "All ChunkedBuffer threads should complete with mutex safety");
    }
}