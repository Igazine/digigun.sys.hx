package digigun.sys.fs;

/**
 * Types of file system events.
 */
enum FSEventType {
    Created;
    Modified;
    Deleted;
    Renamed;
}

/**
 * Structure representing a file system event.
 */
typedef FSEvent = {
    var path:String;
    var type:FSEventType;
    var isDir:Bool;
}
