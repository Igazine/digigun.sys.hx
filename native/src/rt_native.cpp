#include "rt_native.h"

#ifdef _WIN32
#include <windows.h>

extern "C" {

int rt_mlockall(int current, int future) {
    // Windows: SetProcessWorkingSetSize can increase the minimum and maximum working set sizes
    // This isn't quite the same as mlockall, but it's the closest equivalent
    HANDLE hProcess = GetCurrentProcess();
    SIZE_T minSize, maxSize;
    if (GetProcessWorkingSetSize(hProcess, &minSize, &maxSize)) {
        // Double the sizes as a reasonable "lock" attempt
        if (SetProcessWorkingSetSize(hProcess, minSize * 2, maxSize * 2)) return 0;
    }
    return -1;
}

int rt_munlockall() {
    return 0; // Not really applicable as implemented
}

} // extern "C"

#else
// POSIX (Linux/macOS)
#include <sys/mman.h>
#include <errno.h>

extern "C" {

int rt_mlockall(int current, int future) {
    int flags = 0;
    if (current) flags |= MCL_CURRENT;
    if (future) flags |= MCL_FUTURE;
    
    // mlockall returns 0 on success, -1 on failure
    return mlockall(flags);
}

int rt_munlockall() {
    return munlockall();
}

} // extern "C"
#endif
