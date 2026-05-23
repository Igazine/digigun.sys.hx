#include "buffer_native.h"
#include "digigun_alloc.h"
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <vector>
#include <mutex>

/**
 * RingBuffer Implementation
 */
class RingBuffer {
public:
    RingBuffer(int size) : size(size), head(0), tail(0), count(0) {
        data = (unsigned char*)malloc(size);
    }
    ~RingBuffer() { free(data); }

    int write(const void* src, int len) {
        std::lock_guard<std::mutex> lock(mutex);
        int free_space = size - count;
        int to_write = std::min(len, free_space);
        if (to_write <= 0) return 0;

        int first_part = std::min(to_write, size - tail);
        memcpy(data + tail, src, first_part);
        if (to_write > first_part) {
            memcpy(data, (unsigned char*)src + first_part, to_write - first_part);
        }

        tail = (tail + to_write) % size;
        count += to_write;
        return to_write;
    }

    int read(void* dest, int len, bool peek) {
        std::lock_guard<std::mutex> lock(mutex);
        int to_read = std::min(len, count);
        if (to_read <= 0) return 0;

        int first_part = std::min(to_read, size - head);
        if (dest) {
            memcpy(dest, data + head, first_part);
            if (to_read > first_part) {
                memcpy((unsigned char*)dest + first_part, data, to_read - first_part);
            }
        }

        if (!peek) {
            head = (head + to_read) % size;
            count -= to_read;
        }
        return to_read;
    }

    int skip(int len) {
        return read(nullptr, len, false);
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex);
        head = tail = count = 0;
    }

    int available() { return count; }
    int free_space() { return size - count; }

private:
    unsigned char* data;
    int size;
    int head;
    int tail;
    int count;
    std::mutex mutex;
};

/**
 * BipBuffer Implementation
 */
class BipBuffer {
public:
    BipBuffer(int size) : size(size), res_head(0), res_size(0),
                          a_head(0), a_tail(0), b_head(0), b_tail(0), is_b(false) {
        data = (unsigned char*)malloc(size);
    }
    ~BipBuffer() { free(data); }

    void* reserve(int len, int* reserved_len) {
        std::lock_guard<std::mutex> lock(mutex);
        if (is_b) {
            // Writing to Region B
            int free_space = a_head - b_tail;
            int to_res = std::min(len, free_space);
            if (to_res <= 0) { *reserved_len = 0; return nullptr; }
            res_head = b_tail;
            res_size = to_res;
            *reserved_len = to_res;
            return data + res_head;
        } else {
            // Writing to Region A
            int free_space = size - a_tail;
            if (free_space >= len || free_space >= a_head) {
                // Stay in A
                int to_res = std::min(len, free_space);
                res_head = a_tail;
                res_size = to_res;
                *reserved_len = to_res;
                return data + res_head;
            } else {
                // Switch to B
                is_b = true;
                int to_res = std::min(len, a_head);
                res_head = 0;
                res_size = to_res;
                *reserved_len = to_res;
                return data + res_head;
            }
        }
    }

    void commit(int len) {
        std::lock_guard<std::mutex> lock(mutex);
        if (len > res_size) len = res_size;
        if (is_b) {
            b_tail += len;
        } else {
            a_tail += len;
        }
        res_size = 0;
    }

    void* get_read_ptr(int* available_len) {
        std::lock_guard<std::mutex> lock(mutex);
        if (a_head < a_tail) {
            *available_len = a_tail - a_head;
            return data + a_head;
        } else if (b_head < b_tail) {
            *available_len = b_tail - b_head;
            return data + b_head;
        }
        *available_len = 0;
        return nullptr;
    }

    void decommit(int len) {
        std::lock_guard<std::mutex> lock(mutex);
        if (a_head < a_tail) {
            a_head += len;
            if (a_head == a_tail) {
                a_head = a_tail = 0;
                // If B exists, it now becomes A
                if (b_tail > 0) {
                    a_head = b_head;
                    a_tail = b_tail;
                    b_head = b_tail = 0;
                    is_b = false;
                }
            }
        } else if (b_head < b_tail) {
            b_head += len;
            if (b_head == b_tail) {
                b_head = b_tail = 0;
                is_b = false;
            }
        }
    }

    int available() {
        std::lock_guard<std::mutex> lock(mutex);
        return (a_tail - a_head) + (b_tail - b_head);
    }

    int free_space() {
        std::lock_guard<std::mutex> lock(mutex);
        if (is_b) return a_head - b_tail;
        return (size - a_tail) + a_head;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex);
        a_head = a_tail = b_head = b_tail = 0;
        is_b = false;
        res_size = 0;
    }

private:
    unsigned char* data;
    int size;
    int a_head, a_tail;
    int b_head, b_tail;
    bool is_b;
    int res_head, res_size;
    std::mutex mutex;
};

/**
 * ChunkedBuffer Implementation
 */
struct NativeChunk {
    unsigned char* data;
    int size;
    int used;
    NativeChunk* next;
};

class ChunkedBuffer {
public:
    ChunkedBuffer(int chunk_size) : chunk_size(chunk_size), head_chunk(nullptr), tail_chunk(nullptr), total_available(0) {}
    ~ChunkedBuffer() { clear(); }

    int write(const void* src, int len) {
        std::lock_guard<std::mutex> lock(mutex);
        int remaining = len;
        const unsigned char* src_ptr = (const unsigned char*)src;

        while (remaining > 0) {
            if (!tail_chunk || tail_chunk->used == tail_chunk->size) {
                NativeChunk* new_chunk = (NativeChunk*)malloc(sizeof(NativeChunk));
                new_chunk->data = (unsigned char*)malloc(chunk_size);
                new_chunk->size = chunk_size;
                new_chunk->used = 0;
                new_chunk->next = nullptr;
                if (!head_chunk) head_chunk = new_chunk;
                if (tail_chunk) tail_chunk->next = new_chunk;
                tail_chunk = new_chunk;
            }

            int space = tail_chunk->size - tail_chunk->used;
            int to_write = std::min(remaining, space);
            memcpy(tail_chunk->data + tail_chunk->used, src_ptr, to_write);
            tail_chunk->used += to_write;
            src_ptr += to_write;
            remaining -= to_write;
            total_available += to_write;
        }
        return len;
    }

    int read(void* dest, int len, bool peek) {
        std::lock_guard<std::mutex> lock(mutex);
        int to_read = std::min(len, total_available);
        if (to_read <= 0) return 0;

        int remaining = to_read;
        unsigned char* dest_ptr = (unsigned char*)dest;
        NativeChunk* current = head_chunk;
        int read_total = 0;

        while (remaining > 0 && current) {
            int available = current->used;
            int taking = std::min(remaining, available);
            if (dest_ptr) {
                memcpy(dest_ptr, current->data, taking);
                dest_ptr += taking;
            }
            read_total += taking;
            remaining -= taking;

            if (!peek) {
                current->used -= taking;
                total_available -= taking;
                if (current->used > 0) {
                    memmove(current->data, current->data + taking, current->used);
                }

                if (current->used == 0) {
                    NativeChunk* next = current->next;
                    free(current->data);
                    free(current);
                    head_chunk = next;
                    if (!head_chunk) tail_chunk = nullptr;
                    current = next;
                } else {
                    current = current->next;
                }
            } else {
                current = current->next;
            }
        }
        return read_total;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex);
        NativeChunk* c = head_chunk;
        while (c) {
            NativeChunk* next = c->next;
            free(c->data);
            free(c);
            c = next;
        }
        head_chunk = tail_chunk = nullptr;
        total_available = 0;
    }

    int available() { return total_available; }

private:
    int chunk_size;
    NativeChunk* head_chunk;
    NativeChunk* tail_chunk;
    int total_available;
    std::mutex mutex;
};

extern "C" {

/** RingBuffer Exports */
long long ring_buffer_create(int size) {
    RingBuffer* rb = new RingBuffer(size);
    if (rb) digigun::g_active_allocations++;
    return (long long)(size_t)rb;
}
void ring_buffer_destroy(long long handle) {
    RingBuffer* rb = (RingBuffer*)(size_t)handle;
    if (rb) {
        delete rb;
        digigun::g_active_allocations--;
    }
}
int ring_buffer_write(long long handle, const void* data, int len) { return ((RingBuffer*)(size_t)handle)->write(data, len); }
int ring_buffer_read(long long handle, void* data, int len) { return ((RingBuffer*)(size_t)handle)->read(data, len, false); }
int ring_buffer_peek(long long handle, void* data, int len) { return ((RingBuffer*)(size_t)handle)->read(data, len, true); }
int ring_buffer_skip(long long handle, int len) { return ((RingBuffer*)(size_t)handle)->skip(len); }
int ring_buffer_available(long long handle) { return ((RingBuffer*)(size_t)handle)->available(); }
int ring_buffer_free_space(long long handle) { return ((RingBuffer*)(size_t)handle)->free_space(); }
void ring_buffer_clear(long long handle) { ((RingBuffer*)(size_t)handle)->clear(); }

/** BipBuffer Exports */
long long bip_buffer_create(int size) {
    BipBuffer* bb = new BipBuffer(size);
    if (bb) digigun::g_active_allocations++;
    return (long long)(size_t)bb;
}
void bip_buffer_destroy(long long handle) {
    BipBuffer* bb = (BipBuffer*)(size_t)handle;
    if (bb) {
        delete bb;
        digigun::g_active_allocations--;
    }
}
void* bip_buffer_reserve(long long handle, int len, int* reserved_len) { return ((BipBuffer*)(size_t)handle)->reserve(len, reserved_len); }
void bip_buffer_commit(long long handle, int len) { ((BipBuffer*)(size_t)handle)->commit(len); }
void* bip_buffer_get_read_ptr(long long handle, int* available_len) { return ((BipBuffer*)(size_t)handle)->get_read_ptr(available_len); }
void bip_buffer_decommit(long long handle, int len) { ((BipBuffer*)(size_t)handle)->decommit(len); }
int bip_buffer_write(long long handle, const void* data, int len) {
    int res = 0;
    void* p = ((BipBuffer*)(size_t)handle)->reserve(len, &res);
    if (p && res > 0) {
        memcpy(p, data, res);
        ((BipBuffer*)(size_t)handle)->commit(res);
        return res;
    }
    return 0;
}
int bip_buffer_read(long long handle, void* data, int len) {
    int res = 0;
    void* p = ((BipBuffer*)(size_t)handle)->get_read_ptr(&res);
    if (p && res > 0) {
        int to_read = std::min(len, res);
        memcpy(data, p, to_read);
        ((BipBuffer*)(size_t)handle)->decommit(to_read);
        return to_read;
    }
    return 0;
}
int bip_buffer_peek(long long handle, void* data, int len) {
    int res = 0;
    void* p = ((BipBuffer*)(size_t)handle)->get_read_ptr(&res);
    if (p && res > 0) {
        int to_read = std::min(len, res);
        memcpy(data, p, to_read);
        return to_read;
    }
    return 0;
}
int bip_buffer_skip(long long handle, int len) {
    int res = 0;
    void* p = ((BipBuffer*)(size_t)handle)->get_read_ptr(&res);
    if (p && res > 0) {
        int to_skip = std::min(len, res);
        ((BipBuffer*)(size_t)handle)->decommit(to_skip);
        return to_skip;
    }
    return 0;
}
int bip_buffer_available(long long handle) { return ((BipBuffer*)(size_t)handle)->available(); }
int bip_buffer_free_space(long long handle) { return ((BipBuffer*)(size_t)handle)->free_space(); }
void bip_buffer_clear(long long handle) { ((BipBuffer*)(size_t)handle)->clear(); }

/** ChunkedBuffer Exports */
long long chunked_buffer_create(int chunk_size) {
    ChunkedBuffer* cb = new ChunkedBuffer(chunk_size);
    if (cb) digigun::g_active_allocations++;
    return (long long)(size_t)cb;
}
void chunked_buffer_destroy(long long handle) {
    ChunkedBuffer* cb = (ChunkedBuffer*)(size_t)handle;
    if (cb) {
        delete cb;
        digigun::g_active_allocations--;
    }
}
int chunked_buffer_write(long long handle, const void* data, int len) { return ((ChunkedBuffer*)(size_t)handle)->write(data, len); }
int chunked_buffer_read(long long handle, void* data, int len) { return ((ChunkedBuffer*)(size_t)handle)->read(data, len, false); }
int chunked_buffer_peek(long long handle, void* data, int len) { return ((ChunkedBuffer*)(size_t)handle)->read(data, len, true); }
int chunked_buffer_available(long long handle) { return ((ChunkedBuffer*)(size_t)handle)->available(); }
void chunked_buffer_clear(long long handle) { ((ChunkedBuffer*)(size_t)handle)->clear(); }

} // extern "C"
