#ifndef DL_NATIVE_H
#define DL_NATIVE_H

#include <stddef.h>
#include "digigun_export.h"

extern "C" {
    /**
     * Loads a shared library.
     * @param path Path to the library (.so, .dylib, .dll).
     * @return A handle to the library, or 0 on failure.
     */
    DIGIGUN_API long long dl_open(const char* path);

    /**
     * Retrieves the address of a exported symbol.
     * @param handle The library handle returned by dl_open.
     * @param name The symbol name.
     * @return The address of the symbol, or 0 if not found.
     */
    DIGIGUN_API long long dl_get_symbol(long long handle, const char* name);

    /**
     * Unloads a shared library.
     * @param handle The library handle.
     */
    DIGIGUN_API void dl_close(long long handle);

    /**
     * Returns the last error message from the dynamic loader.
     * @return Error string, or NULL if no error.
     */
    DIGIGUN_API const char* dl_get_error();
}

#endif // DL_NATIVE_H
