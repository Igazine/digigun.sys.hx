#ifndef CONSOLE_NATIVE_H
#define CONSOLE_NATIVE_H

#include <stddef.h>
#include "digigun_export.h"

extern "C" {
    DIGIGUN_API int console_set_raw_mode(int enable);
    DIGIGUN_API void console_get_size(int* width, int* height);
    DIGIGUN_API void console_write_block(int x, int y, int width, int height, const int* chars, const int* attrs);
    DIGIGUN_API void console_read_block(int x, int y, int width, int height, int* chars, int* attrs);
    DIGIGUN_API void console_set_cursor_visible(int visible);
    DIGIGUN_API void console_use_alternate_buffer(int enable);
}

#endif // CONSOLE_NATIVE_H
