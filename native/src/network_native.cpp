#include "network_native.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <windows.h>
#include <netioapi.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

static void ensure_winsock_init() {
    static bool initialized = false;
    if (!initialized) {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        initialized = true;
    }
}

extern "C" {

NativeIP* network_get_host_info(const char* host) {
    ensure_winsock_init();
    struct addrinfo hints, *res, *p;
    char ipstr[INET6_ADDRSTRLEN];
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, NULL, &hints, &res) != 0) return NULL;
    NativeIP* head = NULL; NativeIP* tail = NULL;
    for (p = res; p != NULL; p = p->ai_next) {
        void* addr;
        if (p->ai_family == AF_INET) addr = &(((struct sockaddr_in*)p->ai_addr)->sin_addr);
        else addr = &(((struct sockaddr_in6*)p->ai_addr)->sin6_addr);
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        NativeIP* node = (NativeIP*)malloc(sizeof(NativeIP));
        if (!node) { network_free_host_info(head); freeaddrinfo(res); return NULL; }
        strncpy(node->ip, ipstr, 255); node->ip[255] = '\0'; node->next = NULL;
        if (!head) head = node;
        if (tail) tail->next = node;
        tail = node;
    }
    freeaddrinfo(res); return head;
}

void network_free_host_info(NativeIP* list) {
    while (list) { NativeIP* next = list->next; free(list); list = next; }
}

NativeInterface* network_get_interfaces() {
    ensure_winsock_init();
    ULONG outBufLen = 15000;
    PIP_ADAPTER_ADDRESSES pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
    if (!pAddresses) return NULL;
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST;
    DWORD dwRetVal = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAddresses, &outBufLen);
    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        free(pAddresses); pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
        if (!pAddresses) return NULL;
        dwRetVal = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAddresses, &outBufLen);
    }
    if (dwRetVal != NO_ERROR) { free(pAddresses); return NULL; }
    NativeInterface* head = NULL; NativeInterface* tail = NULL;
    for (PIP_ADAPTER_ADDRESSES pCurr = pAddresses; pCurr != NULL; pCurr = pCurr->Next) {
        NativeInterface* ni = (NativeInterface*)malloc(sizeof(NativeInterface));
        if (!ni) { network_free_interfaces(head); free(pAddresses); return NULL; }
        memset(ni, 0, sizeof(NativeInterface));
        WideCharToMultiByte(CP_ACP, 0, pCurr->FriendlyName, -1, ni->name, 255, NULL, NULL);
        ni->flags = 0;
        if (pCurr->OperStatus == IfOperStatusUp) ni->flags |= 0x1;
        if (pCurr->IfType == IF_TYPE_SOFTWARE_LOOPBACK) ni->flags |= 0x8;
        if (pCurr->PhysicalAddressLength == 6) {
            snprintf(ni->mac, 255, "%02x:%02x:%02x:%02x:%02x:%02x",
                    pCurr->PhysicalAddress[0], pCurr->PhysicalAddress[1], pCurr->PhysicalAddress[2],
                    pCurr->PhysicalAddress[3], pCurr->PhysicalAddress[4], pCurr->PhysicalAddress[5]);
        }
        for (PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurr->FirstUnicastAddress; pUnicast != NULL; pUnicast = pUnicast->Next) {
            char addrStr[INET6_ADDRSTRLEN];
            if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
                inet_ntop(AF_INET, &((struct sockaddr_in*)pUnicast->Address.lpSockaddr)->sin_addr, addrStr, sizeof(addrStr));
                strncpy(ni->ipv4, addrStr, 255);
            } else if (pUnicast->Address.lpSockaddr->sa_family == AF_INET6) {
                inet_ntop(AF_INET6, &((struct sockaddr_in6*)pUnicast->Address.lpSockaddr)->sin6_addr, addrStr, sizeof(addrStr));
                strncpy(ni->ipv6, addrStr, 255);
            }
        }
        ni->next = NULL; if (!head) head = ni; if (tail) tail->next = ni; tail = ni;
    }
    free(pAddresses); return head;
}

void network_free_interfaces(NativeInterface* list) {
    while (list) { NativeInterface* next = list->next; free(list); list = next; }
}

double network_ping(const char* host, int timeout_ms) {
    ensure_winsock_init();
    HANDLE hIcmpFile = IcmpCreateFile();
    if (hIcmpFile == INVALID_HANDLE_VALUE) return -1.0;
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    if (getaddrinfo(host, NULL, &hints, &res) != 0) { IcmpCloseHandle(hIcmpFile); return -1.0; }
    IPAddr ipaddr = ((struct sockaddr_in*)res->ai_addr)->sin_addr.S_un.S_addr;
    freeaddrinfo(res);
    char sendData[] = "Data Buffer";
    DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData) + 8;
    void* replyBuffer = malloc(replySize);
    if (!replyBuffer) { IcmpCloseHandle(hIcmpFile); return -1.0; }
    DWORD dwRetVal = IcmpSendEcho(hIcmpFile, ipaddr, sendData, sizeof(sendData), NULL, replyBuffer, replySize, timeout_ms);
    double rtt = -1.0;
    if (dwRetVal != 0) {
        PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)replyBuffer;
        if (pEchoReply->Status == IP_SUCCESS) rtt = (double)pEchoReply->RoundTripTime;
    }
    free(replyBuffer); IcmpCloseHandle(hIcmpFile); return rtt;
}

NativeArpEntry* network_get_arp_table() {
    ensure_winsock_init();
    PMIB_IPNET_TABLE2 table;
    if (GetIpNetTable2(AF_UNSPEC, &table) != NO_ERROR) return NULL;

    NativeArpEntry* head = NULL; NativeArpEntry* tail = NULL;
    for (ULONG i = 0; i < table->NumEntries; i++) {
        NativeArpEntry* node = (NativeArpEntry*)malloc(sizeof(NativeArpEntry));
        if (!node) break;
        memset(node, 0, sizeof(NativeArpEntry));

        // IP
        char ip[INET6_ADDRSTRLEN];
        if (table->Table[i].Address.si_family == AF_INET)
            inet_ntop(AF_INET, &table->Table[i].Address.Ipv4.sin_addr, ip, sizeof(ip));
        else
            inet_ntop(AF_INET6, &table->Table[i].Address.Ipv6.sin6_addr, ip, sizeof(ip));
        strncpy(node->ip, ip, 255);

        // MAC
        snprintf(node->mac, 255, "%02x:%02x:%02x:%02x:%02x:%02x",
                table->Table[i].PhysicalAddress[0], table->Table[i].PhysicalAddress[1],
                table->Table[i].PhysicalAddress[2], table->Table[i].PhysicalAddress[3],
                table->Table[i].PhysicalAddress[4], table->Table[i].PhysicalAddress[5]);

        node->next = NULL;
        if (!head) head = node; else tail->next = node; tail = node;
    }
    FreeMibTable(table);
    return head;
}

void network_free_arp_table(NativeArpEntry* list) {
    while (list) { NativeArpEntry* next = list->next; free(list); list = next; }
}

int network_bind_to_interface(int socket_fd, const char* interface_name) {
    // Windows implementation: find the local IP assigned to this interface and bind to it.
    ensure_winsock_init();
    // This is complex on Windows as it doesn't have a direct SO_BINDTODEVICE.
    // We'd need to iterate adapters, match name, and get unicast IP.
    return -1; // Not yet implemented robustly for Windows
}

} // extern "C"

#else
// POSIX Implementation
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/time.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>

#ifdef __APPLE__
    #include <sys/sysctl.h>
    #include <net/if_dl.h>
#endif

#include <net/route.h>

#ifdef __linux__
    #include <linux/if_packet.h>
    #include <net/ethernet.h>
#else
    #include <netinet/if_ether.h>
    #include <net/if_arp.h>
    #include <net/if_dl.h>
#endif

extern "C" {

NativeIP* network_get_host_info(const char* host) {
    struct addrinfo hints, *res, *p;
    char ipstr[INET6_ADDRSTRLEN];
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, NULL, &hints, &res) != 0) return NULL;
    NativeIP* head = NULL; NativeIP* tail = NULL;
    for (p = res; p != NULL; p = p->ai_next) {
        void* addr;
        if (p->ai_family == AF_INET) addr = &(((struct sockaddr_in*)p->ai_addr)->sin_addr);
        else addr = &(((struct sockaddr_in6*)p->ai_addr)->sin6_addr);
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        NativeIP* node = (NativeIP*)malloc(sizeof(NativeIP));
        if (!node) { network_free_host_info(head); freeaddrinfo(res); return NULL; }
        strncpy(node->ip, ipstr, 255); node->ip[255] = '\0'; node->next = NULL;
        if (!head) head = node; if (tail) tail->next = node; tail = node;
    }
    freeaddrinfo(res); return head;
}

void network_free_host_info(NativeIP* list) {
    while (list) { NativeIP* next = list->next; free(list); list = next; }
}

NativeInterface* network_get_interfaces() {
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) return NULL;
    NativeInterface* head = NULL; NativeInterface* tail = NULL;
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;
        NativeInterface* ni = NULL; NativeInterface* curr = head;
        while (curr) { if (strcmp(curr->name, ifa->ifa_name) == 0) { ni = curr; break; } curr = curr->next; }
        if (!ni) {
            ni = (NativeInterface*)malloc(sizeof(NativeInterface));
            if (!ni) { network_free_interfaces(head); freeifaddrs(ifaddr); return NULL; }
            memset(ni, 0, sizeof(NativeInterface));
            strncpy(ni->name, ifa->ifa_name, 255); ni->flags = ifa->ifa_flags; ni->next = NULL;
            if (!head) head = ni; if (tail) tail->next = ni; tail = ni;
        }
        int family = ifa->ifa_addr->sa_family;
        if (family == AF_INET || family == AF_INET6) {
            char host[NI_MAXHOST];
            int s = getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
                               host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s == 0) { if (family == AF_INET) strncpy(ni->ipv4, host, 255); else strncpy(ni->ipv6, host, 255); }
        } 
#ifdef __linux__
        else if (family == AF_PACKET) {
            struct sockaddr_ll *sll = (struct sockaddr_ll *)ifa->ifa_addr;
            if (sll->sll_halen == 6) snprintf(ni->mac, 255, "%02x:%02x:%02x:%02x:%02x:%02x", sll->sll_addr[0], sll->sll_addr[1], sll->sll_addr[2], sll->sll_addr[3], sll->sll_addr[4], sll->sll_addr[5]);
        }
#else
        else if (family == AF_LINK) {
            struct sockaddr_dl *sdl = (struct sockaddr_dl *)ifa->ifa_addr;
            if (sdl->sdl_alen == 6) { unsigned char* ptr = (unsigned char *)LLADDR(sdl); snprintf(ni->mac, 255, "%02x:%02x:%02x:%02x:%02x:%02x", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]); }
        }
#endif
    }
    freeifaddrs(ifaddr); return head;
}

void network_free_interfaces(NativeInterface* list) {
    while (list) { NativeInterface* next = list->next; free(list); list = next; }
}

static uint16_t calculate_checksum(void *b, int len) {
    uint16_t *buf = (uint16_t *)b; unsigned int sum = 0;
    for (sum = 0; len > 1; len -= 2) sum += *buf++;
    if (len == 1) sum += *(uint8_t *)buf;
    sum = (sum >> 16) + (sum & 0xFFFF); sum += (sum >> 16); return (uint16_t)(~sum);
}

double network_ping(const char* host, int timeout_ms) {
    struct addrinfo hints, *res; struct sockaddr_in dest_addr;
    memset(&hints, 0, sizeof(hints)); hints.ai_family = AF_INET; hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(host, NULL, &hints, &res) != 0) return -1.0;
    memset(&dest_addr, 0, sizeof(dest_addr)); dest_addr.sin_family = AF_INET;
    memcpy(&dest_addr.sin_addr, &((struct sockaddr_in*)res->ai_addr)->sin_addr, sizeof(struct in_addr));
    freeaddrinfo(res);
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
    if (sock < 0) return -1.0;
    struct timeval tv_out; tv_out.tv_sec = timeout_ms / 1000; tv_out.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_out, sizeof(tv_out));
    struct icmp icp; memset(&icp, 0, sizeof(icp)); icp.icmp_type = ICMP_ECHO; icp.icmp_code = 0;
    icp.icmp_id = getpid() & 0xFFFF; icp.icmp_seq = 1; icp.icmp_cksum = calculate_checksum(&icp, sizeof(icp));
    struct timeval start, end; gettimeofday(&start, NULL);
    if (sendto(sock, &icp, sizeof(icp), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) <= 0) { close(sock); return -1.0; }
    char buffer[1024]; struct sockaddr_in from_addr; socklen_t from_len = sizeof(from_addr);
    if (recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&from_addr, &from_len) <= 0) { close(sock); return -1.0; }
    gettimeofday(&end, NULL);
    struct icmp* reply; if (buffer[0] == ICMP_ECHOREPLY) reply = (struct icmp*)buffer; else reply = (struct icmp*)(buffer + 20);
    if (reply->icmp_type != ICMP_ECHOREPLY) { close(sock); return -1.0; }
    close(sock);
    return (double)(end.tv_sec - start.tv_sec) * 1000.0 + (double)(end.tv_usec - start.tv_usec) / 1000.0;
}

NativeArpEntry* network_get_arp_table() {
#ifdef __linux__
    FILE* f = fopen("/proc/net/arp", "r");
    if (!f) return NULL;
    char line[512];
    fgets(line, sizeof(line), f); // skip header
    NativeArpEntry* head = NULL; NativeArpEntry* tail = NULL;
    while (fgets(line, sizeof(line), f)) {
        char ip[64], hw[64], flags[64], mac[64], mask[64], dev[64];
        if (sscanf(line, "%s %s %s %s %s %s", ip, hw, flags, mac, mask, dev) == 6) {
            NativeArpEntry* node = (NativeArpEntry*)malloc(sizeof(NativeArpEntry));
            memset(node, 0, sizeof(NativeArpEntry));
            strncpy(node->ip, ip, 255); strncpy(node->mac, mac, 255); strncpy(node->interface_name, dev, 255);
            if (!head) head = node; else tail->next = node; tail = node;
        }
    }
    fclose(f); return head;
#elif defined(__APPLE__)
    // Simple macOS implementation via sysctl
    int mib[6]; size_t needed; char *lim, *buf, *next; struct rt_msghdr *rtm; struct sockaddr_inarp *sin; struct sockaddr_dl *sdl;
    mib[0] = CTL_NET; mib[1] = PF_ROUTE; mib[2] = 0; mib[3] = AF_INET; mib[4] = NET_RT_FLAGS; mib[5] = RTF_LLINFO;
    if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0) return NULL;
    if ((buf = (char*)malloc(needed)) == NULL) return NULL;
    if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0) { free(buf); return NULL; }
    lim = buf + needed;
    NativeArpEntry* head = NULL; NativeArpEntry* tail = NULL;
    for (next = buf; next < lim; next += rtm->rtm_msglen) {
        rtm = (struct rt_msghdr *)next;
        sin = (struct sockaddr_inarp *)(rtm + 1);
        sdl = (struct sockaddr_dl *)(sin + 1);
        if (sdl->sdl_alen) {
            NativeArpEntry* node = (NativeArpEntry*)malloc(sizeof(NativeArpEntry));
            memset(node, 0, sizeof(NativeArpEntry));
            inet_ntop(AF_INET, &sin->sin_addr, node->ip, sizeof(node->ip));
            unsigned char* ptr = (unsigned char *)LLADDR(sdl);
            snprintf(node->mac, 255, "%02x:%02x:%02x:%02x:%02x:%02x", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
            if_indextoname(sdl->sdl_index, node->interface_name);
            if (!head) head = node; else tail->next = node; tail = node;
        }
    }
    free(buf); return head;
#endif
    return NULL;
}

void network_free_arp_table(NativeArpEntry* list) {
    while (list) { NativeArpEntry* next = list->next; free(list); list = next; }
}

int network_bind_to_interface(int socket_fd, const char* interface_name) {
#ifdef __linux__
    return setsockopt(socket_fd, SOL_SOCKET, SO_BINDTODEVICE, interface_name, strlen(interface_name));
#elif defined(__APPLE__)
    unsigned int ifindex = if_nametoindex(interface_name);
    if (ifindex == 0) return -1;
    return setsockopt(socket_fd, IPPROTO_IP, IP_BOUND_IF, &ifindex, sizeof(ifindex));
#endif
    return -1;
}

} // extern "C"
#endif
