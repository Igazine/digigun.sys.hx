package battlefield;

import utest.Test;
import utest.Assert;
import utest.Async;
import digigun.sys.io.NativeLoop;
import digigun.sys.io.AsyncFile;
import digigun.sys.io.NativeBuffer;
import haxe.io.Bytes;
import sys.FileSystem;

class TestNativeLoop extends Test {
    var loop:NativeLoop;
    var tempPath:String = "temp_stress_file.bin";

    public function setup() {
        loop = new NativeLoop();
    }

    public function teardown() {
        if (loop != null) {
            loop.close();
        }
        if (FileSystem.exists(tempPath)) {
            FileSystem.deleteFile(tempPath);
        }
    }

    public function testRapidAsyncOperations(async:Async) {
        var iterations = 500; // Async operations are heavier
        var completed = 0;
        
        // Pre-create a file to read from
        var content = Bytes.alloc(1024);
        sys.io.File.saveBytes(tempPath, content);
        
        var file = AsyncFile.open(loop, tempPath, false);
        Assert.notNull(file);

        trace('Submitting $iterations rapid async read operations...');
        
        for (i in 0...iterations) {
            file.read(1024, (result, data) -> {
                completed++;
                if (data != null) data.free();
                
                if (completed == iterations) {
                    file.close();
                    async.done();
                }
            });
        }

        // Run the loop in a background-like manner or poll it here
        // In a real test we might need a timer or a loop
        haxe.Timer.delay(function() {
            var start = haxe.Timer.stamp();
            while (completed < iterations && (haxe.Timer.stamp() - start) < 10.0) {
                loop.poll(10);
            }
            if (completed < iterations) {
                Assert.fail("Timed out waiting for async operations to complete");
                async.done();
            }
        }, 0);
    }
    
    public function testLoopStabilityUnderLoad() {
        // High frequency polling with no operations
        for (i in 0...1000) {
            loop.poll(0);
        }
        Assert.pass();
    }
}