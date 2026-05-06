#ifndef AUTH_NATIVE_H
#define AUTH_NATIVE_H

#include <stddef.h>
#include "digigun_export.h"

/**
 * Native structure representing user information.
 */
struct NativeUserInfo {
    int uid;
    int gid;
    const char* username;
    const char* realname;
    const char* home_dir;
    const char* shell;
    struct NativeUserInfo* next;
};

/**
 * Native structure representing group information.
 */
struct NativeGroupInfo {
    int gid;
    const char* name;
    struct NativeGroupInfo* next;
};

extern "C" {
    DIGIGUN_API struct NativeUserInfo* auth_get_current_user();
    DIGIGUN_API struct NativeUserInfo* auth_get_user_by_uid(int uid);
    DIGIGUN_API struct NativeUserInfo* auth_get_user_by_name(const char* name);
    DIGIGUN_API void auth_free_user(struct NativeUserInfo* user);

    DIGIGUN_API struct NativeGroupInfo* auth_get_group_by_gid(int gid);
    DIGIGUN_API struct NativeGroupInfo* auth_get_groups();
    DIGIGUN_API void auth_free_groups(struct NativeGroupInfo* groups);
}

#endif // AUTH_NATIVE_H
