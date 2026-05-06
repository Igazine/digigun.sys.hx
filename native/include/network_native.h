#ifndef NETWORK_NATIVE_H
#define NETWORK_NATIVE_H

#include <stddef.h>
#include "digigun_export.h"

/**
 * Native interface structure.
 */
struct NativeInterface {
    char name[256];
    char ipv4[256];
    char ipv6[256];
    char mac[256];
    int flags;
    struct NativeInterface* next;
};

/**
 * Native structure for ping replies in a session.
 */
struct NativePingReply {
    int seq;
    double rtt;
    struct NativePingReply* next;
};

/**
 * Declares network system calls.
 */
extern "C" {
    /**
     * Resolves a host to a linked list of IP strings.
     */
    struct NativeIP {
        char ip[256];
        struct NativeIP* next;
    };

    DIGIGUN_API struct NativeIP* network_get_host_info(const char* host);
    DIGIGUN_API void network_free_host_info(struct NativeIP* list);

    DIGIGUN_API struct NativeInterface* network_get_interfaces();
    DIGIGUN_API void network_free_interfaces(struct NativeInterface* list);

    DIGIGUN_API double network_ping(const char* host, int timeout_ms);

    struct NativeArpEntry {
        char ip[256];
        char mac[256];
        char interface_name[256];
        struct NativeArpEntry* next;
    };

    DIGIGUN_API struct NativeArpEntry* network_get_arp_table();
    DIGIGUN_API void network_free_arp_table(struct NativeArpEntry* list);

    DIGIGUN_API int network_bind_to_interface(int socket_fd, const char* interface_name);

    /**
     * Advanced Socket Control
     */
    DIGIGUN_API int network_set_socket_opt(int socket_fd, int level, int option, const void* val, int len);
    DIGIGUN_API int network_get_constant(const char* name);

    /**
     * Mass Ping (Session-based)
     */
    DIGIGUN_API long long ping_session_open();
    DIGIGUN_API int ping_session_send(long long handle, const char* host, int seq);
    DIGIGUN_API struct NativePingReply* ping_session_recv(long long handle);
    DIGIGUN_API void ping_session_free_replies(struct NativePingReply* list);
    DIGIGUN_API void ping_session_close(long long handle);
}

#endif // NETWORK_NATIVE_H
