#include "random_native.h"
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#else
#include <stdio.h>
#include <unistd.h>
#endif

extern "C" {

int random_get_bytes(unsigned char* buffer, size_t size) {
#ifdef _WIN32
    NTSTATUS status = BCryptGenRandom(NULL, buffer, (ULONG)size, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    return (status == 0) ? 0 : -1;
#else
    FILE* f = fopen("/dev/urandom", "rb");
    if (!f) return -1;
    size_t res = fread(buffer, 1, size, f);
    fclose(f);
    return (res == size) ? 0 : -1;
#endif
}

} // extern "C"
