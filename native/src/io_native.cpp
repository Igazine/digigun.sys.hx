#include "io_native.h"
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <termios.h>
#include <stdint.h>
#ifdef __APPLE__
#include <sys/uio.h>
#else
#include <sys/sendfile.h>
#endif
#endif

extern "C" {

long long io_sendfile(long long out_handle, const char* path, long long offset, long long length) {
#ifdef _WIN32
    // Windows implementation using TransmitFile or fallback
    HANDLE hOut = (HANDLE)(size_t)out_handle;
    HANDLE hIn = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hIn == INVALID_HANDLE_VALUE) return -1;

    LARGE_INTEGER liOffset;
    liOffset.QuadPart = offset;
    OVERLAPPED overlapped = {0};
    overlapped.Offset = liOffset.LowPart;
    overlapped.OffsetHigh = liOffset.HighPart;

    BOOL success = TransmitFile((SOCKET)hOut, hIn, (DWORD)length, 0, &overlapped, NULL, 0);
    CloseHandle(hIn);
    return success ? length : -1;
#elif defined(__APPLE__)
    int out_fd = (int)(size_t)out_handle;
    int in_fd = open(path, O_RDONLY);
    if (in_fd < 0) return -1;

    off_t len = length;
    int res = sendfile(in_fd, out_fd, offset, &len, NULL, 0);
    close(in_fd);
    return (res == 0) ? (long long)len : -1;
#else
    int out_fd = (int)(size_t)out_handle;
    int in_fd = open(path, O_RDONLY);
    if (in_fd < 0) return -1;

    off_t off = offset;
    ssize_t res = sendfile(out_fd, in_fd, &off, length);
    close(in_fd);
    return (long long)res;
#endif
}

int io_set_direct_io(long long handle, int enabled) {
#ifdef _WIN32
    // Windows handle flags cannot be changed after creation easily
    return -1;
#elif defined(__APPLE__)
    int fd = (int)(size_t)handle;
    return fcntl(fd, F_NOCACHE, enabled ? 1 : 0);
#else
    // Linux requires O_DIRECT during open()
    return -1;
#endif
}

long long io_open_file(const char* path, int write_mode) {
#ifdef _WIN32
    DWORD access = GENERIC_READ;
    if (write_mode) access |= GENERIC_WRITE;
    HANDLE h = CreateFileA(path, access, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
    return (long long)(size_t)h;
#else
    int flags = write_mode ? (O_RDWR | O_CREAT) : O_RDONLY;
    int fd = open(path, flags, 0644);
    return (long long)(size_t)fd;
#endif
}

void io_close_file(long long handle) {
#ifdef _WIN32
    CloseHandle((HANDLE)(size_t)handle);
#else
    close((int)handle);
#endif
}

long long buffer_alloc(int size) {
    return (long long)(size_t)malloc(size);
}

void buffer_free(long long handle) {
    free((void*)(size_t)handle);
}

void* buffer_get_ptr(long long handle) {
    return (void*)(size_t)handle;
}

int io_mem_protect(void* addr, size_t len, int flags) {
#ifdef _WIN32
    DWORD win_prot = 0;
    // flags: READ=1, WRITE=2, EXEC=4
    if (flags & 4) { // EXEC
        if (flags & 2) win_prot = PAGE_EXECUTE_READWRITE;
        else if (flags & 1) win_prot = PAGE_EXECUTE_READ;
        else win_prot = PAGE_EXECUTE;
    } else {
        if (flags & 2) win_prot = PAGE_READWRITE;
        else if (flags & 1) win_prot = PAGE_READONLY;
        else win_prot = PAGE_NOACCESS;
    }

    DWORD old;
    if (VirtualProtect(addr, len, win_prot, &old)) return 0;
    return -1;
#else
#include <stdint.h>
    int posix_prot = PROT_NONE;
    if (flags & 1) posix_prot |= PROT_READ;
    if (flags & 2) posix_prot |= PROT_WRITE;
    if (flags & 4) posix_prot |= PROT_EXEC;

    // Align to page boundary for mprotect
    size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
    uintptr_t uaddr = (uintptr_t)addr;
    uintptr_t aligned_addr = uaddr & ~(page_size - 1);
    size_t aligned_len = len + (uaddr - aligned_addr);

    if (mprotect((void*)aligned_addr, aligned_len, posix_prot) == 0) return 0;
    return -1;
#endif
}

int io_mem_pagesize() {
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (int)si.dwPageSize;
#else
    return (int)sysconf(_SC_PAGESIZE);
#endif
}

long long io_serial_open(const char* port_name, int baud_rate, int data_bits, int parity, int stop_bits) {
#ifdef _WIN32
    char full_name[256];
    if (strncmp(port_name, "COM", 3) == 0 && strlen(port_name) > 4) {
        snprintf(full_name, sizeof(full_name), "\\\\.\\%s", port_name);
    } else {
        strncpy(full_name, port_name, sizeof(full_name));
    }

    HANDLE h = CreateFileA(full_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) return 0;

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(h, &dcbSerialParams)) { CloseHandle(h); return 0; }

    dcbSerialParams.BaudRate = baud_rate;
    dcbSerialParams.ByteSize = data_bits;
    dcbSerialParams.StopBits = (stop_bits == 1) ? ONESTOPBIT : TWOSTOPBITS;
    dcbSerialParams.Parity = (parity == 0) ? NOPARITY : (parity == 1 ? ODDPARITY : EVENPARITY);

    if (!SetCommState(h, &dcbSerialParams)) { CloseHandle(h); return 0; }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = MAXDWORD; // Non-blocking style
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    SetCommTimeouts(h, &timeouts);

    return (long long)(size_t)h;
#else
    int fd = open(port_name, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) return 0;

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) { close(fd); return 0; }

    speed_t speed = B9600;
    switch(baud_rate) {
        case 9600: speed = B9600; break;
        case 19200: speed = B19200; break;
        case 38400: speed = B38400; break;
        case 57600: speed = B57600; break;
        case 115200: speed = B115200; break;
        default: speed = B9600; break;
    }
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | (data_bits == 8 ? CS8 : CS7);
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 5;

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_cflag |= (CLOCAL | CREAD);

    // Parity
    if (parity == 0) tty.c_cflag &= ~(PARENB | PARODD);
    else if (parity == 1) { tty.c_cflag |= PARENB; tty.c_cflag |= PARODD; }
    else if (parity == 2) { tty.c_cflag |= PARENB; tty.c_cflag &= ~PARODD; }

    // Stop bits
    if (stop_bits == 1) tty.c_cflag &= ~CSTOPB;
    else tty.c_cflag |= CSTOPB;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) { close(fd); return 0; }
    return (long long)(size_t)fd;
#endif
}

int io_serial_read(long long handle, void* buffer, int length) {
#ifdef _WIN32
    DWORD bytesRead;
    if (ReadFile((HANDLE)(size_t)handle, buffer, (DWORD)length, &bytesRead, NULL)) return (int)bytesRead;
    return -1;
#else
    return (int)read((int)handle, buffer, length);
#endif
}

int io_serial_write(long long handle, const void* buffer, int length) {
#ifdef _WIN32
    DWORD bytesWritten;
    if (WriteFile((HANDLE)(size_t)handle, buffer, (DWORD)length, &bytesWritten, NULL)) return (int)bytesWritten;
    return -1;
#else
    return (int)write((int)handle, buffer, length);
#endif
}

void io_serial_close(long long handle) {
#ifdef _WIN32
    CloseHandle((HANDLE)(size_t)handle);
#else
    close((int)handle);
#endif
}

} // extern "C"
