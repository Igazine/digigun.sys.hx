import utest.Runner;
import utest.ui.Report;
import battlefield.TestNativeBuffers;
import battlefield.TestNativeLoop;
import battlefield.TestIPC;
import battlefield.TestFFI;
import battlefield.TestNetworkMetadata;

class Battlefield {
    public static function main() {
        var runner = new Runner();
        
        // Add test cases as they are implemented
        runner.addCase(new TestNativeBuffers());
        runner.addCase(new TestNativeLoop());
        runner.addCase(new TestIPC());
        runner.addCase(new TestFFI());
        runner.addCase(new TestNetworkMetadata());
        
        Report.create(runner);
        runner.onComplete.add(function(_) {
            haxe.Timer.delay(function() {
                cpp.vm.Gc.run(true);
                var active = digigun.sys.io.NativeBuffer.getActiveAllocations();
                if (active > 0) {
                    trace('FATAL: Memory leak detected! $active active allocations remain.');
                } else {
                    trace('CLEAN: No native memory leaks detected.');
                }
            }, 100);
        });
        runner.run();
    }
}