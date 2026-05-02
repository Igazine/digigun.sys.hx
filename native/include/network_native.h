#ifndef NETWORK_NATIVE_H
#define NETWORK_NATIVE_H

#include <stddef.h>

/**
 * Native interface structure.
 */
struct NativeInterface {
    char name[256];
    char ipv4[256];
    char ipv6[256];
    char mac[256];
    int flags;
    NativeInterface* next;
};

/**
 * Native structure for ping replies in a session.
 */
struct NativePingReply {
    int seq;
    double rtt;
    NativePingReply* next;
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
        NativeIP* next;
    };

    NativeIP* network_get_host_info(const char* host);
    void network_free_host_info(NativeIP* list);

    NativeInterface* network_get_interfaces();
    void network_free_interfaces(NativeInterface* list);

    double network_ping(const char* host, int timeout_ms);

    struct NativeArpEntry {
        char ip[256];
        char mac[256];
        char interface_name[256];
        NativeArpEntry* next;
    };

    NativeArpEntry* network_get_arp_table();
    void network_free_arp_table(NativeArpEntry* list);

    int network_bind_to_interface(int socket_fd, const char* interface_name);

    /**
     * Advanced Socket Control
     */
    int network_set_socket_opt(int socket_fd, int level, int option, const void* val, int len);
    int network_get_constant(const char* name);

    /**
     * Mass Ping (Session-based)
     * Handles are passed as double to avoid HXCPP template ambiguity.
     */
    double ping_session_open();
    int ping_session_send(double handle, const char* host, int seq);
    struct NativePingReply* ping_session_recv(double handle);
    void ping_session_free_replies(struct NativePingReply* list);
    void ping_session_close(double handle);
}

#endif // NETWORK_NATIVE_H
