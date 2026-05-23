package battlefield;

import utest.Test;
import utest.Assert;
import digigun.sys.io.NativeBuffer;
import digigun.sys.io.RingBuffer;
import digigun.sys.io.BipBuffer;
import digigun.sys.io.ChunkedBuffer;
import haxe.io.Bytes;

class TestNativeBuffers extends Test {
    
    public function testHighVolumeAllocation() {
        var iterations = 10000; // Starting with 10k for initial safety, can increase
        trace('Starting 10k allocation stress test...');
        
        for (i in 0...iterations) {
            var buf = new NativeBuffer(1024);
            Assert.notNull(buf);
            Assert.equals(1024, buf.size);
            buf.free();
        }
        Assert.pass();
    }
    
    public function testRingBufferStress() {
        var iterations = 5000;
        var ring = new RingBuffer(4096);
        var data = Bytes.ofString("STRESS_TEST_DATA_PAYLOAD");
        var out = Bytes.alloc(data.length);
        
        for (i in 0...iterations) {
            ring.writeBytes(data, 0, data.length);
            var readCount = ring.readBytes(out, 0, data.length);
            Assert.equals(data.length, readCount);
            Assert.equals("STRESS_TEST_DATA_PAYLOAD", out.toString());
        }
        ring.destroy();
        Assert.pass();
    }
    
    public function testBipBufferStress() {
        var iterations = 5000;
        var bip = new BipBuffer(8192);
        
        for (i in 0...iterations) {
            var res = bip.reserve(1024);
            Assert.notEquals(0, res.address);
            bip.commit(1024);
            
            var read = bip.getReadPtr();
            Assert.equals(1024, read.len);
            bip.decommit(1024);
        }
        bip.destroy();
        Assert.pass();
    }

    public function testGCResilience() {
        trace('Starting GC resilience test...');
        var initial = NativeBuffer.getActiveAllocations();
        trace('Initial active allocations: $initial');
        
        for (i in 0...1000) {
            // Create and drop reference immediately
            var buf = new NativeBuffer(1024 * 1024); // 1MB
            buf = null; 
            
            if (i % 100 == 0) {
                cpp.vm.Gc.run(true); // Force GC
            }
        }
        
        for (i in 0...3) {
            cpp.vm.Gc.run(true);
            Sys.sleep(0.01);
        }
        var current = NativeBuffer.getActiveAllocations();
        trace('Current active allocations after GC: $current');
        
        // If finalization works, we should be back to initial (or close to it)
        Assert.isTrue(current <= initial, 'Memory leaked despite GC! (Before: $initial, After: $current)');
        Assert.pass();
    }

    public function testMetadataStress() {
        trace('Starting metadata allocation stress test (Auth, MemoryMap)...');
        var initial = NativeBuffer.getActiveAllocations();
        
        for (i in 0...100) {
            var user = digigun.sys.auth.Auth.getCurrentUser();
            Assert.notNull(user);
            
            // MemoryMap stress
            var mmap = digigun.sys.fs.MemoryMap.open("test_mmap.bin", 1024, true);
            if (mmap != null) {
                mmap = null; // orphan
            }
            
            if (i % 10 == 0) cpp.vm.Gc.run(true);
        }
        
        for (i in 0...3) {
            cpp.vm.Gc.run(true);
            Sys.sleep(0.01);
        }
        var current = NativeBuffer.getActiveAllocations();
        trace('Current active allocations after metadata GC: $current');
        Assert.isTrue(current <= initial, 'Metadata leaked! (Before: $initial, After: $current)');
    }
}