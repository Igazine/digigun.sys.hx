#include "digigun_alloc.h"

namespace digigun {
    std::atomic<int> g_active_allocations(0);
}
