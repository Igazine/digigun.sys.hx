package digigun.sys.auth;

import cpp.Pointer;

/**
 * User information record.
 */
typedef UserInfo = {
    /** User ID (POSIX only, -1 on Windows). */
    var uid:Int;
    /** Group ID (POSIX only, -1 on Windows). */
    var gid:Int;
    /** Login name. */
    var username:String;
    /** Real name or GECOS field. */
    var realname:String;
    /** Path to home directory. */
    var homeDir:String;
    /** Path to login shell (POSIX only). */
    var shell:String;
}

/**
 * Group information record.
 */
typedef GroupInfo = {
    /** Group ID (POSIX only, -1 on Windows). */
    var gid:Int;
    /** Group name. */
    var name:String;
}

/**
 * Provides access to system identity and credentials.
 */
class Auth {
    /**
     * Retrieves information about the current user.
     * @return UserInfo object or null on failure.
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
     * @param uid The user ID to look up.
     * @return UserInfo object or null if not found.
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
     * Alias for getUserByUid (POSIX) or getUserByName (generic).
     * @param id Either a UID (Int) or a username (String).
     * @return UserInfo object or null if not found.
     */
    public static function getUser(id:Dynamic):UserInfo {
        if (Std.isOfType(id, Int)) {
            return getUserByUid(cast id);
        } else if (Std.isOfType(id, String)) {
            return getUserByName(cast id);
        }
        return null;
    }

    /**
     * Retrieves information about a user by their username.
     * @param name The username to look up.
     * @return UserInfo object or null if not found.
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
     * @return Array of GroupInfo objects.
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

    /**
     * Alias for listGroups.
     * @return Array of GroupInfo objects.
     */
    public static function getGroups():Array<GroupInfo> {
        return listGroups();
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
