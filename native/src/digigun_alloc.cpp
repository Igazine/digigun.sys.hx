#include "digigun_alloc.h"
#include <mutex>

namespace digigun {
    static int _alloc_count = 0;
    static std::mutex _alloc_mutex;

    // We can still use the global extern, but implement it with a mutex if atomic is failing
    // Actually, let's stick to atomic but make sure it's 8-byte aligned
    alignas(8) std::atomic<int> g_active_allocations(0);
}
