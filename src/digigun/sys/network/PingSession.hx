package digigun.sys.network;

import cpp.Pointer;
#if cpp
import cpp.Finalizable;
#end

/**
 * High-performance ping session for simultaneous multi-target pinging.
 * Uses a single persistent raw ICMP socket.
 */
class PingSession #if cpp extends Finalizable #end {
    private var handle:Float = 0;
    private var seqMap:Map<Int, String>;
    private var nextSeq:Int = 1;

    /**
     * Creates and opens a new ping session.
     */
    public function new() {
        #if cpp
        super();
        this.handle = cast Native.ping_session_open();
        this.seqMap = new Map<Int, String>();
        #end
    }

    /**
     * Sends a non-blocking ICMP echo request to a host.
     * @param host The hostname or IP to ping.
     * @return The sequence number assigned to this request, or -1 on error.
     */
    public function send(host:String):Int {
        #if cpp
        if (handle == 0) return -1;
        var seq = nextSeq++;
        if (nextSeq > 65535) nextSeq = 1; // ICMP seq is 16-bit

        if (Native.ping_session_send(cast handle, host, seq) == 0) {
            seqMap.set(seq, host);
            return seq;
        }
        return -1;
        #else
        return -1;
        #end
    }

    /**
     * Non-blockingly collects all available ping replies.
     * @return Array of objects containing host, rtt (ms), and sequence number.
     */
    public function collect():Array<{host:String, rtt:Float, seq:Int}> {
        #if cpp
        if (handle == 0) return [];
        var headRaw = Native.ping_session_recv(cast handle);
        if (headRaw == null) return [];

        var results = [];
        var current = Pointer.fromRaw(headRaw);

        while (current != null && current.raw != null) {
            var reply = current.value;
            var host = seqMap.get(reply.seq);
            if (host != null) {
                results.push({
                    host: host,
                    rtt: reply.rtt,
                    seq: reply.seq
                });
                seqMap.remove(reply.seq);
            }
            current = Pointer.fromRaw(reply.next);
        }

        Native.ping_session_free_replies(headRaw);
        return results;
        #else
        return [];
        #end
    }

    /**
     * Closes the session and releases the raw socket.
     */
    public function close():Void {
        #if cpp
        if (handle != 0) {
            Native.ping_session_close(cast handle);
            handle = 0;
        }
        #end
    }

    #if cpp
    override public function finalize():Void {
        close();
    }
    #end
}
