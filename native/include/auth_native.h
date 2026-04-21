#ifndef AUTH_NATIVE_H
#define AUTH_NATIVE_H

#include <stddef.h>

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
    struct NativeUserInfo* auth_get_user_by_uid(int uid);
    struct NativeUserInfo* auth_get_user_by_name(const char* name);
    struct NativeUserInfo* auth_get_current_user();
    void auth_free_user(struct NativeUserInfo* user);

    struct NativeGroupInfo* auth_get_group_by_gid(int gid);
    struct NativeGroupInfo* auth_get_groups();
    void auth_free_groups(struct NativeGroupInfo* groups);
}

#endif // AUTH_NATIVE_H
