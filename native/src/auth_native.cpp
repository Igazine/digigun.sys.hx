#include "auth_native.h"
#include <string>
#include <vector>
#include <cstring>
#include <stdlib.h>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <lmcons.h>
    #include <sddl.h>
    #include <shlobj.h>
    #define SECURITY_WIN32
    #include <security.h>
    #pragma comment(lib, "secur32.lib")
    #pragma comment(lib, "advapi32.lib")
    #pragma comment(lib, "userenv.lib")
#else
    #include <unistd.h>
    #include <pwd.h>
    #include <grp.h>
    #include <sys/types.h>
#endif

// Helper to duplicate strings safely for Haxe consumption
static const char* safe_strdup(const char* str) {
    if (!str) return (const char*)malloc(1); // Return empty string on null
    size_t len = strlen(str);
    char* res = (char*)malloc(len + 1);
    if (res) {
        memcpy(res, str, len + 1);
    }
    return res;
}

extern "C" {

#ifdef _WIN32
static struct NativeUserInfo* win32_fill_user(const char* name) {
    struct NativeUserInfo* info = (struct NativeUserInfo*)malloc(sizeof(struct NativeUserInfo));
    memset(info, 0, sizeof(struct NativeUserInfo));
    info->uid = -1;
    info->gid = -1;
    info->username = safe_strdup(name);
    info->realname = safe_strdup("");
    info->home_dir = safe_strdup("");
    info->shell = safe_strdup("");
    info->next = NULL;

    // Try to get Full Name
    char fullName[256];
    ULONG nameSize = sizeof(fullName);
    if (GetUserNameExA(NameDisplay, fullName, &nameSize)) {
        if (info->realname) free((void*)info->realname);
        info->realname = safe_strdup(fullName);
    }

    // Try to get Home Directory
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        char path[MAX_PATH];
        DWORD pathSize = MAX_PATH;
        if (GetUserProfileDirectoryA(hToken, path, &pathSize)) {
            if (info->home_dir) free((void*)info->home_dir);
            info->home_dir = safe_strdup(path);
        }
        CloseHandle(hToken);
    }

    // Fallback for home dir
    if (strlen(info->home_dir) == 0) {
        char path[MAX_PATH];
        if (GetEnvironmentVariableA("USERPROFILE", path, MAX_PATH)) {
            if (info->home_dir) free((void*)info->home_dir);
            info->home_dir = safe_strdup(path);
        }
    }
    
    return info;
}
#else
static struct NativeUserInfo* posix_fill_user(struct passwd* pw) {
    if (!pw) return NULL;
    struct NativeUserInfo* info = (struct NativeUserInfo*)malloc(sizeof(struct NativeUserInfo));
    info->uid = pw->pw_uid;
    info->gid = pw->pw_gid;
    info->username = safe_strdup(pw->pw_name);
    info->realname = safe_strdup(pw->pw_gecos);
    info->home_dir = safe_strdup(pw->pw_dir);
    info->shell = safe_strdup(pw->pw_shell);
    info->next = NULL;
    return info;
}
#endif

struct NativeUserInfo* auth_get_user_by_uid(int uid) {
#ifdef _WIN32
    return NULL; // UID not applicable on Windows in this way
#else
    return posix_fill_user(getpwuid((uid_t)uid));
#endif
}

struct NativeUserInfo* auth_get_user_by_name(const char* name) {
#ifdef _WIN32
    return win32_fill_user(name);
#else
    return posix_fill_user(getpwnam(name));
#endif
}

struct NativeUserInfo* auth_get_current_user() {
#ifdef _WIN32
    char name[UNLEN + 1];
    DWORD size = UNLEN + 1;
    if (GetUserNameA(name, &size)) return win32_fill_user(name);
    return NULL;
#else
    return posix_fill_user(getpwuid(getuid()));
#endif
}

void auth_free_user(struct NativeUserInfo* user) {
    if (!user) return;
    if (user->username) free((void*)user->username);
    if (user->realname) free((void*)user->realname);
    if (user->home_dir) free((void*)user->home_dir);
    if (user->shell) free((void*)user->shell);
    free(user);
}

struct NativeGroupInfo* auth_get_group_by_gid(int gid) {
#ifdef _WIN32
    return NULL;
#else
    struct group* gr = getgrgid((gid_t)gid);
    if (!gr) return NULL;
    struct NativeGroupInfo* info = (struct NativeGroupInfo*)malloc(sizeof(struct NativeGroupInfo));
    info->gid = (int)gr->gr_gid;
    info->name = safe_strdup(gr->gr_name);
    info->next = NULL;
    return info;
#endif
}

struct NativeGroupInfo* auth_get_groups() {
#ifdef _WIN32
    return NULL;
#else
    struct NativeGroupInfo* head = NULL;
    struct NativeGroupInfo* tail = NULL;
    
    setgrent();
    struct group* gr;
    while ((gr = getgrent()) != NULL) {
        struct NativeGroupInfo* info = (struct NativeGroupInfo*)malloc(sizeof(struct NativeGroupInfo));
        info->gid = (int)gr->gr_gid;
        info->name = safe_strdup(gr->gr_name);
        info->next = NULL;
        
        if (!head) head = info;
        else tail->next = info;
        tail = info;
    }
    endgrent();
    return head;
#endif
}

void auth_free_groups(struct NativeGroupInfo* groups) {
    while (groups) {
        struct NativeGroupInfo* next = groups->next;
        if (groups->name) free((void*)groups->name);
        free(groups);
        groups = next;
    }
}

} // extern "C"
