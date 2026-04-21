#ifndef NETWORK_NATIVE_H
#define NETWORK_NATIVE_H

#include <string>

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
}

#endif // NETWORK_NATIVE_H
