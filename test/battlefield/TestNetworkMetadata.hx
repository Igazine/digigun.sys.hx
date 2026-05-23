package battlefield;

import utest.Test;
import utest.Assert;
import digigun.sys.network.Network;
import digigun.sys.io.NativeBuffer;

class TestNetworkMetadata extends Test {
    
    public function testInterfaceListingStress() {
        trace('Starting Network Interface listing stress test...');
        var initial = NativeBuffer.getActiveAllocations();
        
        for (i in 0...50) {
            var interfaces = Network.getNetworkInterfaces();
            // drop reference
            interfaces = null;
            
            if (i % 10 == 0) cpp.vm.Gc.run(true);
        }
        
        cpp.vm.Gc.run(true);
        var current = NativeBuffer.getActiveAllocations();
        trace('Current active allocations after Network GC: $current');
        // Network.getInterfaces() might return many interfaces, each with many IP nodes.
        // If it leaked, current would be much higher than initial.
        Assert.isTrue(current <= initial, 'Network metadata leaked! (Before: $initial, After: $current)');
    }
}