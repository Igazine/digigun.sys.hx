#include "network_native.h"
#include "digigun_alloc.h"
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

#ifdef __APPLE__
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/bpf.h>
#endif

#ifdef __linux__
#include <sys/ioctl.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/if.h>
#endif

#ifdef _WIN32
#include <mstcpip.h>
#endif

// Mass Ping Session management
struct PingSession {
    int socket_fd;
    std::mutex mtx;
};

static std::map<long long, PingSession*> g_ping_sessions;
static std::mutex g_ping_sessions_mtx;
static long long g_ping_session_id = 1;

/**
 * Raw Sniffer State
 */
struct RawSniffer {
    int fd;
    int buf_len;
#ifdef _WIN32
    SOCKET sock;
#endif
};

static std::map<long long, RawSniffer*> g_sniffers;
static std::mutex g_sniffers_mtx;
static long long g_sniffer_id = 1;

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
        if (node) {
            memset(node, 0, sizeof(NativeIP));
            digigun::g_active_allocations++;
        } else {
            freeaddrinfo(res);
            return NULL;
        }
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
        digigun::g_active_allocations--;
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
        if (node) {
            memset(node, 0, sizeof(NativeInterface));
            digigun::g_active_allocations++;
        } else {
            freeifaddrs(ifaddr);
            return head;
        }
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
            snprintf(node->mac, sizeof(node->mac), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
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
        digigun::g_active_allocations--;
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
        digigun::g_active_allocations--;
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

int network_set_socket_opt(int socket_fd, int level, int option, const void* val, int len) {
    return setsockopt(socket_fd, level, option, (const char*)val, len);
}

long long ping_session_open() {
    PingSession* session = new PingSession();
    if (session) digigun::g_active_allocations++;
    session->socket_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    
    std::lock_guard<std::mutex> lock(g_ping_sessions_mtx);
    long long id = g_ping_session_id++;
    g_ping_sessions[id] = session;
    return id;
}

int ping_session_send(long long handle, const char* host, int seq) {
    std::lock_guard<std::mutex> lock(g_ping_sessions_mtx);
    if (g_ping_sessions.find(handle) == g_ping_sessions.end()) return -1;
    PingSession* session = g_ping_sessions[handle];
    // Implement ICMP send logic...
    return 0;
}

struct NativePingReply* ping_session_recv(long long handle) {
    return NULL;
}

void ping_session_free_replies(struct NativePingReply* list) {
    while (list) {
        NativePingReply* next = list->next;
        free(list);
        list = next;
    }
}

void ping_session_close(long long handle) {
    std::lock_guard<std::mutex> lock(g_ping_sessions_mtx);
    if (g_ping_sessions.find(handle) != g_ping_sessions.end()) {
        PingSession* session = g_ping_sessions[handle];
        if (session->socket_fd >= 0) {
#ifdef _WIN32
            closesocket(session->socket_fd);
#else
            close(session->socket_fd);
#endif
        }
        delete session;
        digigun::g_active_allocations--;
        g_ping_sessions.erase(handle);
    }
}

long long network_raw_sniffer_open(const char* interface_name, int promiscuous) {
    RawSniffer* sniffer = new RawSniffer();
    if (sniffer) digigun::g_active_allocations++;
    sniffer->buf_len = 65536;

#ifdef __APPLE__
    // macOS: Berkeley Packet Filter
    char bpf_dev[16];
    sniffer->fd = -1;
    for (int i = 0; i < 99; i++) {
        snprintf(bpf_dev, sizeof(bpf_dev), "/dev/bpf%d", i);
        sniffer->fd = open(bpf_dev, O_RDWR);
        if (sniffer->fd != -1) break;
        if (errno != EBUSY) break;
    }
    if (sniffer->fd == -1) { delete sniffer; digigun::g_active_allocations--; return 0; }

    // Set buffer length
    ioctl(sniffer->fd, BIOCSBLEN, &sniffer->buf_len);
    
    // Bind to interface
    struct ifreq ifr;
    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ);
    if (ioctl(sniffer->fd, BIOCSETIF, &ifr) == -1) { close(sniffer->fd); delete sniffer; digigun::g_active_allocations--; return 0; }

    // Immediate mode (don't buffer)
    unsigned int enable = 1;
    ioctl(sniffer->fd, BIOCIMMEDIATE, &enable);
    
    if (promiscuous) ioctl(sniffer->fd, BIOCPROMISC, NULL);

#elif defined(_WIN32)
    // Windows: SIO_RCVALL
    sniffer->sock = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
    if (sniffer->sock == INVALID_SOCKET) { delete sniffer; digigun::g_active_allocations--; return 0; }

    sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = inet_addr(interface_name); // On Windows, user must provide local IP
    local.sin_port = htons(0);

    if (bind(sniffer->sock, (sockaddr*)&local, sizeof(local)) == SOCKET_ERROR) {
        closesocket(sniffer->sock); delete sniffer; digigun::g_active_allocations--; return 0;
    }

    int optval = promiscuous ? RCVALL_ON : RCVALL_OFF;
    DWORD dwBytesRet = 0;
    if (WSAIoctl(sniffer->sock, SIO_RCVALL, &optval, sizeof(optval), NULL, 0, &dwBytesRet, NULL, NULL) == SOCKET_ERROR) {
        closesocket(sniffer->sock); delete sniffer; digigun::g_active_allocations--; return 0;
    }
    sniffer->fd = (int)sniffer->sock;

#else
    // Linux: AF_PACKET
    sniffer->fd = socket(AF_PACKET, SOCK_RAW, htons(0x0003)); // ETH_P_ALL = 0x0003
    if (sniffer->fd == -1) { delete sniffer; digigun::g_active_allocations--; return 0; }

    struct ifreq ifr;
    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ);
    if (ioctl(sniffer->fd, SIOCGIFINDEX, &ifr) == -1) { close(sniffer->fd); delete sniffer; digigun::g_active_allocations--; return 0; }

    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifr.ifr_ifindex;
    sll.sll_protocol = htons(0x0003);

    if (bind(sniffer->fd, (struct sockaddr*)&sll, sizeof(sll)) == -1) { close(sniffer->fd); delete sniffer; digigun::g_active_allocations--; return 0; }

    if (promiscuous) {
        struct packet_mreq mr;
        memset(&mr, 0, sizeof(mr));
        mr.mr_ifindex = ifr.ifr_ifindex;
        mr.mr_type = PACKET_MR_PROMISC;
        setsockopt(sniffer->fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr));
    }
#endif

    std::lock_guard<std::mutex> lock(g_sniffers_mtx);
    long long id = g_sniffer_id++;
    g_sniffers[id] = sniffer;
    return id;
}

int network_raw_sniffer_read(long long handle, void* buffer, int length) {
    std::lock_guard<std::mutex> lock(g_sniffers_mtx);
    if (g_sniffers.find(handle) == g_sniffers.end()) return -1;
    RawSniffer* sniffer = g_sniffers[handle];

#ifdef __APPLE__
    // BPF returns packets wrapped in bpf_hdr
    static char bpf_internal_buf[65536];
    int bytes = read(sniffer->fd, bpf_internal_buf, sizeof(bpf_internal_buf));
    if (bytes <= 0) return bytes;
    
    struct bpf_hdr* hdr = (struct bpf_hdr*)bpf_internal_buf;
    int packet_len = hdr->bh_caplen;
    if (packet_len > length) packet_len = length;
    memcpy(buffer, bpf_internal_buf + hdr->bh_hdrlen, packet_len);
    return packet_len;
#else
    return recv(sniffer->fd, (char*)buffer, length, 0);
#endif
}

void network_raw_sniffer_close(long long handle) {
    std::lock_guard<std::mutex> lock(g_sniffers_mtx);
    if (g_sniffers.find(handle) != g_sniffers.end()) {
        RawSniffer* sniffer = g_sniffers[handle];
#ifdef _WIN32
        closesocket(sniffer->sock);
#else
        close(sniffer->fd);
#endif
        delete sniffer;
        digigun::g_active_allocations--;
        g_sniffers.erase(handle);
    }
}

} // extern "C"
