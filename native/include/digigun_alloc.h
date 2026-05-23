#ifndef DIGIGUN_ALLOC_H
#define DIGIGUN_ALLOC_H

#include <atomic>

namespace digigun {
    extern std::atomic<int> g_active_allocations;
}

#endif
