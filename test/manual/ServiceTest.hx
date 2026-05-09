package;

import digigun.sys.service.Service;
import sys.io.File;
import haxe.io.Path;

class ServiceTest {
    static var logFile:String = "C:\\service_haxe.log";

    static function log(msg:String) {
        try {
            var f = File.append(logFile, false);
            f.writeString('[HAXE] $msg\n');
            f.close();
        } catch(e:Dynamic) {}
    }

    static function main() {
        var args = Sys.args();
        log("ServiceTest starting with args: " + args.join(" "));

        if (args.indexOf("--service") != -1) {
            log("Running as service...");
            var res = Service.run("digigun_test", onStart, onStop);
            log('Service.run returned: $res');
        } else {
            log("Running as standalone app.");
            onStart();
            Sys.sleep(5);
            onStop();
        }
    }

    static function onStart() {
        log("onStart called!");
        // Keep doing something in a background thread if needed,
        // or just let the service block.
    }

    static function onStop() {
        log("onStop called!");
    }
}
