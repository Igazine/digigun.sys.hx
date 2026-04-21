package digigun.sys.auth;

import cpp.Pointer;

typedef UserInfo = {
    var uid:Int;
    var gid:Int;
    var username:String;
    var realname:String;
    var homeDir:String;
    var shell:String;
}

typedef GroupInfo = {
    var gid:Int;
    var name:String;
}

/**
 * Provides access to system identity and credentials.
 */
class Auth {
    /**
     * Retrieves information about the current user.
     */
    public static function getCurrentUser():UserInfo {
        #if cpp
        var raw = Native.get_current_user();
        if (raw == null) return null;
        var info = convertUser(raw);
        Native.free_user(raw);
        return info;
        #else
        return null;
        #end
    }

    /**
     * Retrieves information about a user by their UID (POSIX only).
     */
    public static function getUserByUid(uid:Int):UserInfo {
        #if cpp
        var raw = Native.get_user_by_uid(uid);
        if (raw == null) return null;
        var info = convertUser(raw);
        Native.free_user(raw);
        return info;
        #else
        return null;
        #end
    }

    /**
     * Retrieves information about a user by their username.
     */
    public static function getUserByName(name:String):UserInfo {
        #if cpp
        var raw = Native.get_user_by_name(name);
        if (raw == null) return null;
        var info = convertUser(raw);
        Native.free_user(raw);
        return info;
        #else
        return null;
        #end
    }

    /**
     * Retrieves all groups on the system (POSIX only).
     */
    public static function listGroups():Array<GroupInfo> {
        #if cpp
        var headRaw = Native.get_groups();
        if (headRaw == null) return [];

        var result:Array<GroupInfo> = [];
        var current = Pointer.fromRaw(headRaw);

        while (current != null && current.raw != null) {
            var g = current.value;
            result.push({
                gid: g.gid,
                name: g.name.toString()
            });
            current = Pointer.fromRaw(g.next);
        }

        Native.free_groups(headRaw);
        return result;
        #else
        return [];
        #end
    }

    #if cpp
    private static function convertUser(raw:cpp.RawPointer<Native.NativeUserInfo>):UserInfo {
        var p = Pointer.fromRaw(raw).value;
        return {
            uid: p.uid,
            gid: p.gid,
            username: p.username.toString(),
            realname: p.realname.toString(),
            homeDir: p.home_dir.toString(),
            shell: p.shell.toString()
        };
    }
    #end
}
