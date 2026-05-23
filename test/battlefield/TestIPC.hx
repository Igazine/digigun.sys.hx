package battlefield;

import utest.Test;
import utest.Assert;
import digigun.sys.fifo.Fifo;
import digigun.sys.fifo.UnixDomainSocket;
import digigun.sys.shm.SharedMemory;
import haxe.io.Bytes;
import sys.FileSystem;

class TestIPC extends Test {
    
    public function testSharedMemoryChurn() {
        var iterations = 1000;
        var shmName = "stress_test_shm";
        
        trace('Starting $iterations iterations of SharedMemory mapping churn...');
        for (i in 0...iterations) {
            var shm = new SharedMemory();
            if (shm.open(shmName, 4096, true)) {
                shm.write(0, Bytes.ofString("DATA"));
                shm.close();
            }
        }
        Assert.pass();
    }
    
    public function testFifoChurn() {
        var iterations = 500;
        var fifoPath = "stress_test_fifo";
        
        // Clean up previous
        if (FileSystem.exists(fifoPath)) {
            try { FileSystem.deleteFile(fifoPath); } catch(e:Dynamic) {}
        }
        
        trace('Starting $iterations iterations of Fifo creation/deletion churn...');
        for (i in 0...iterations) {
            var fifo = new Fifo();
            if (fifo.create(fifoPath, 438)) { // 0666
                // We don't necessarily need to open it every time, just create/destroy
            }
            // Cleanup for next iteration
            if (FileSystem.exists(fifoPath)) {
                try { FileSystem.deleteFile(fifoPath); } catch(e:Dynamic) {}
            }
        }
        Assert.pass();
    }
    
    public function testUnixSocketChurn() {
        var iterations = 500;
        var socketPath = "stress_test_socket";
        
        if (FileSystem.exists(socketPath)) {
            try { FileSystem.deleteFile(socketPath); } catch(e:Dynamic) {}
        }
        
        trace('Starting $iterations iterations of UnixDomainSocket binding churn...');
        for (i in 0...iterations) {
            var sock = UnixDomainSocket.create();
            try {
                sock.bind(socketPath);
                sock.close();
            } catch(e:Dynamic) {
                sock.close();
            }
            
            if (FileSystem.exists(socketPath)) {
                try { FileSystem.deleteFile(socketPath); } catch(e:Dynamic) {}
            }
        }
        Assert.pass();
    }
}