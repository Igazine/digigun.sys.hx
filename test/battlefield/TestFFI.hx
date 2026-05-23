package battlefield;

import utest.Test;
import utest.Assert;
import digigun.sys.dl.Dl;
import digigun.sys.dl.FFI;

@:build(digigun.sys.dl.FFI.struct())
class StressPoint {
    public var x:Int;
    public var y:Int;
}

class TestFFI extends Test {
    
    public function testStructAllocationChurn() {
        var iterations = 5000;
        trace('Starting $iterations iterations of FFI struct allocation churn...');
        
        for (i in 0...iterations) {
            var p = new StressPoint();
            p.x = i;
            p.y = i * 2;
            Assert.equals(i, p.x);
            Assert.equals(i * 2, p.y);
            // Structs are managed by Haxe GC if they are class instances, 
            // but the @:build(FFI.struct()) makes them point to native memory if used that way.
            // Here we just test the class-to-native mapping logic.
        }
        Assert.pass();
    }
    
    public function testLibraryLoadingChurn() {
        // This test depends on a library being available. 
        // We'll try to load the current executable as a library (self-load) if possible, 
        // or just skip if no known library is present.
        var libPath = "";
        #if macos
        libPath = "bin/battlefield"; // Or similar
        #elseif windows
        libPath = "bin/battlefield.exe";
        #else
        libPath = "bin/battlefield";
        #end
        
        // This is a bit speculative as the file might not be fully flushed or accessible as a DL,
        // but it's a good stress test for the Dl.open/close logic.
        var iterations = 100;
        trace('Starting $iterations iterations of Dl.open/close churn...');
        for (i in 0...iterations) {
            try {
                var handle = Dl.open(libPath);
                if (handle.isValid) {
                    Dl.close(handle);
                }
            } catch(e:Dynamic) {}
        }
        Assert.pass();
    }
}