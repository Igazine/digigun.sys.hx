#include "random_native.h"

#ifdef _WIN32
    #include <windows.h>
    #include <bcrypt.h>
    #pragma comment(lib, "bcrypt.lib")
#else
    #include <fcntl.h>
    #include <unistd.h>
#endif

extern "C" {

int random_get_bytes(unsigned char* buffer, int length) {
#ifdef _WIN32
    if (BCryptGenRandom(NULL, buffer, (ULONG)length, BCRYPT_USE_SYSTEM_PREFERRED_RNG) >= 0) {
        return 0;
    }
    return -1;
#else
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) return -1;
    
    int total = 0;
    while (total < length) {
        ssize_t result = read(fd, buffer + total, length - total);
        if (result < 0) {
            close(fd);
            return -1;
        }
        total += (int)result;
    }
    
    close(fd);
    return 0;
#endif
}

} // extern "C"
