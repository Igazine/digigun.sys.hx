#include "network_native.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <map>
#include <mutex>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#ifdef __APPLE__
#include <net/if_dl.h>
#else
#include <linux/if_packet.h>
#endif
#endif

// Mass Ping Session management
struct PingSession {
    int socket_fd;
    std::mutex mtx;
};

static std::map<double, PingSession*> g_ping_sessions;
static std::mutex g_ping_sessions_mtx;
static double g_ping_session_id = 1.0;

extern "C" {

NativeIP* network_get_host_info(const char* host) {
    struct addrinfo hints, *res, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, NULL, &hints, &res) != 0) return NULL;

    NativeIP* head = NULL;
    NativeIP* tail = NULL;

    for (p = res; p != NULL; p = p->ai_next) {
        char ip[256];
        if (p->ai_family == AF_INET) {
            struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
            inet_ntop(AF_INET, &(ipv4->sin_addr), ip, sizeof(ip));
        } else {
            struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
            inet_ntop(AF_INET6, &(ipv6->sin6_addr), ip, sizeof(ip));
        }

        NativeIP* node = (NativeIP*)malloc(sizeof(NativeIP));
        strncpy(node->ip, ip, 255);
        node->next = NULL;

        if (tail) tail->next = node;
        else head = node;
        tail = node;
    }

    freeaddrinfo(res);
    return head;
}

void network_free_host_info(NativeIP* list) {
    while (list) {
        NativeIP* next = list->next;
        free(list);
        list = next;
    }
}

NativeInterface* network_get_interfaces() {
#ifdef _WIN32
    // Windows implementation using GetAdaptersAddresses
    return NULL;
#else
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) return NULL;

    NativeInterface* head = NULL;
    NativeInterface* tail = NULL;

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;

        NativeInterface* node = (NativeInterface*)malloc(sizeof(NativeInterface));
        memset(node, 0, sizeof(NativeInterface));
        strncpy(node->name, ifa->ifa_name, 255);
        node->flags = ifa->ifa_flags;

        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in* ipv4 = (struct sockaddr_in*)ifa->ifa_addr;
            inet_ntop(AF_INET, &(ipv4->sin_addr), node->ipv4, 255);
        } else if (ifa->ifa_addr->sa_family == AF_INET6) {
            struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)ifa->ifa_addr;
            inet_ntop(AF_INET6, &(ipv6->sin6_addr), node->ipv6, 255);
        }

#ifdef __APPLE__
        if (ifa->ifa_addr->sa_family == AF_LINK) {
            struct sockaddr_dl* sdl = (struct sockaddr_dl*)ifa->ifa_addr;
            unsigned char* mac = (unsigned char*)LLADDR(sdl);
            sprintf(node->mac, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        }
#endif

        if (tail) tail->next = node;
        else head = node;
        tail = node;
    }

    freeifaddrs(ifaddr);
    return head;
#endif
}

void network_free_interfaces(NativeInterface* list) {
    while (list) {
        NativeInterface* next = list->next;
        free(list);
        list = next;
    }
}

double network_ping(const char* host, int timeout_ms) {
#ifdef _WIN32
    // Windows implementation using IcmpSendEcho
    return -1.0;
#else
    // POSIX simplified ping (requires root usually)
    return -1.0;
#endif
}

NativeArpEntry* network_get_arp_table() {
    return NULL;
}

void network_free_arp_table(NativeArpEntry* list) {
    while (list) {
        NativeArpEntry* next = list->next;
        free(list);
        list = next;
    }
}

int network_bind_to_interface(int socket_fd, const char* interface_name) {
#ifdef _WIN32
    return -1;
#else
#ifdef SO_BINDTODEVICE
    return setsockopt(socket_fd, SOL_SOCKET, SO_BINDTODEVICE, interface_name, strlen(interface_name));
#else
    return -1;
#endif
#endif
}

int network_set_socket_opt(int socket_fd, int level, int option, const void* val, int len) {
    return setsockopt(socket_fd, level, option, (const char*)val, len);
}

int network_get_constant(const char* name) {
    if (strcmp(name, "SOL_SOCKET") == 0) return SOL_SOCKET;
#ifdef SO_TIMESTAMP
    if (strcmp(name, "SO_TIMESTAMP") == 0) return SO_TIMESTAMP;
#endif
#ifdef TCP_NODELAY
    if (strcmp(name, "TCP_NODELAY") == 0) return TCP_NODELAY;
#endif
    return -1;
}

double ping_session_open() {
    PingSession* session = new PingSession();
    session->socket_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    
    std::lock_guard<std::mutex> lock(g_ping_sessions_mtx);
    double id = g_ping_session_id++;
    g_ping_sessions[id] = session;
    return id;
}

int ping_session_send(double handle, const char* host, int seq) {
    std::lock_guard<std::mutex> lock(g_ping_sessions_mtx);
    if (g_ping_sessions.find(handle) == g_ping_sessions.end()) return -1;
    PingSession* session = g_ping_sessions[handle];
    // Implement ICMP send logic...
    return 0;
}

struct NativePingReply* ping_session_recv(double handle) {
    return NULL;
}

void ping_session_free_replies(struct NativePingReply* list) {
    while (list) {
        NativePingReply* next = list->next;
        free(list);
        list = next;
    }
}

void ping_session_close(double handle) {
    std::lock_guard<std::mutex> lock(g_ping_sessions_mtx);
    if (g_ping_sessions.find(handle) != g_ping_sessions.end()) {
        PingSession* session = g_ping_sessions[handle];
        if (session->socket_fd >= 0) close(session->socket_fd);
        delete session;
        g_ping_sessions.erase(handle);
    }
}

} // extern "C"
