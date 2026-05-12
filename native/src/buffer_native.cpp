#include "buffer_native.h"
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
    BipBuffer(int size) : size(size), res_head(0), res_tail(0), res_size(0),
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
            if (free_space >= a_head && free_space >= len) {
                res_head = a_tail;
                res_size = len;
                *reserved_len = len;
                return data + res_head;
            } else if (a_head > free_space) {
                // Better to start Region B
                int b_space = a_head;
                int to_res = std::min(len, b_space);
                if (to_res <= 0) { *reserved_len = 0; return nullptr; }
                res_head = 0;
                res_size = to_res;
                *reserved_len = to_res;
                return data + res_head;
            } else {
                // Stay in A but limited
                if (free_space <= 0) { *reserved_len = 0; return nullptr; }
                res_head = a_tail;
                res_size = free_space;
                *reserved_len = free_space;
                return data + res_head;
            }
        }
    }

    void commit(int len) {
        std::lock_guard<std::mutex> lock(mutex);
        if (len <= 0) { res_size = 0; return; }
        int to_commit = std::min(len, res_size);
        if (res_head == a_tail && !is_b) {
            a_tail += to_commit;
        } else {
            is_b = true;
            b_tail += to_commit;
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
            if (a_head >= a_tail) {
                // Region A empty, flip B to A
                a_head = b_head;
                a_tail = b_tail;
                b_head = b_tail = 0;
                is_b = false;
            }
        } else {
            b_head += len;
        }
    }

    int available() {
        return (a_tail - a_head) + (b_tail - b_head);
    }

    int free_space() {
        if (is_b) return a_head - b_tail;
        return std::max(size - a_tail, a_head);
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex);
        a_head = a_tail = b_head = b_tail = res_size = 0;
        is_b = false;
    }

private:
    unsigned char* data;
    int size;
    int res_head, res_tail, res_size;
    int a_head, a_tail;
    int b_head, b_tail;
    bool is_b;
    std::mutex mutex;
};

/**
 * ChunkedBuffer Implementation
 */
struct NativeChunk {
    unsigned char* data;
    int size;
    int head;
    int tail;
    NativeChunk* next;
};

class ChunkedBuffer {
public:
    ChunkedBuffer(int chunk_size) : chunk_size(chunk_size), head_chunk(nullptr), tail_chunk(nullptr), total_available(0) {}
    ~ChunkedBuffer() { clear(); }

    int write(const void* src, int len) {
        std::lock_guard<std::mutex> lock(mutex);
        int written = 0;
        const unsigned char* p = (const unsigned char*)src;

        while (written < len) {
            if (!tail_chunk || tail_chunk->tail >= tail_chunk->size) {
                NativeChunk* chunk = (NativeChunk*)malloc(sizeof(NativeChunk));
                chunk->data = (unsigned char*)malloc(chunk_size);
                chunk->size = chunk_size;
                chunk->head = chunk->tail = 0;
                chunk->next = nullptr;
                if (!head_chunk) head_chunk = chunk;
                if (tail_chunk) tail_chunk->next = chunk;
                tail_chunk = chunk;
            }

            int to_copy = std::min(len - written, tail_chunk->size - tail_chunk->tail);
            memcpy(tail_chunk->data + tail_chunk->tail, p + written, to_copy);
            tail_chunk->tail += to_copy;
            written += to_copy;
        }
        total_available += written;
        return written;
    }

    int read(void* dest, int len, bool peek) {
        std::lock_guard<std::mutex> lock(mutex);
        int read_total = 0;
        unsigned char* d = (unsigned char*)dest;
        NativeChunk* current = head_chunk;

        while (current && read_total < len) {
            int to_copy = std::min(len - read_total, current->tail - current->head);
            if (dest) memcpy(d + read_total, current->data + current->head, to_copy);
            read_total += to_copy;
            
            if (!peek) {
                current->head += to_copy;
                total_available -= to_copy;
                if (current->head >= current->tail) {
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
long long ring_buffer_create(int size) { return (long long)(size_t)new RingBuffer(size); }
void ring_buffer_destroy(long long handle) { delete (RingBuffer*)(size_t)handle; }
int ring_buffer_write(long long handle, const void* data, int len) { return ((RingBuffer*)(size_t)handle)->write(data, len); }
int ring_buffer_read(long long handle, void* data, int len) { return ((RingBuffer*)(size_t)handle)->read(data, len, false); }
int ring_buffer_peek(long long handle, void* data, int len) { return ((RingBuffer*)(size_t)handle)->read(data, len, true); }
int ring_buffer_skip(long long handle, int len) { return ((RingBuffer*)(size_t)handle)->skip(len); }
int ring_buffer_available(long long handle) { return ((RingBuffer*)(size_t)handle)->available(); }
int ring_buffer_free_space(long long handle) { return ((RingBuffer*)(size_t)handle)->free_space(); }
void ring_buffer_clear(long long handle) { ((RingBuffer*)(size_t)handle)->clear(); }

/** BipBuffer Exports */
long long bip_buffer_create(int size) { return (long long)(size_t)new BipBuffer(size); }
void bip_buffer_destroy(long long handle) { delete (BipBuffer*)(size_t)handle; }
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
int bip_buffer_available(long long handle) { return ((BipBuffer*)(size_t)handle)->available(); }
int bip_buffer_free_space(long long handle) { return ((BipBuffer*)(size_t)handle)->free_space(); }
void bip_buffer_clear(long long handle) { ((BipBuffer*)(size_t)handle)->clear(); }

/** ChunkedBuffer Exports */
long long chunked_buffer_create(int chunk_size) { return (long long)(size_t)new ChunkedBuffer(chunk_size); }
void chunked_buffer_destroy(long long handle) { delete (ChunkedBuffer*)(size_t)handle; }
int chunked_buffer_write(long long handle, const void* data, int len) { return ((ChunkedBuffer*)(size_t)handle)->write(data, len); }
int chunked_buffer_read(long long handle, void* data, int len) { return ((ChunkedBuffer*)(size_t)handle)->read(data, len, false); }
int chunked_buffer_peek(long long handle, void* data, int len) { return ((ChunkedBuffer*)(size_t)handle)->read(data, len, true); }
int chunked_buffer_available(long long handle) { return ((ChunkedBuffer*)(size_t)handle)->available(); }
void chunked_buffer_clear(long long handle) { ((ChunkedBuffer*)(size_t)handle)->clear(); }

} // extern "C"
