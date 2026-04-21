#ifndef CONSOLE_NATIVE_H
#define CONSOLE_NATIVE_H

/**
 * Declares Terminal and Console control functions.
 */
extern "C" {
    /**
     * Toggles Raw Mode for the terminal.
     * @param enabled 1 to enable, 0 to disable.
     * @return 1 on success, 0 on failure.
     */
    int console_set_raw_mode(int enabled);

    /**
     * Gets the terminal width and height.
     */
    void console_get_size(int* width, int* height);

    /**
     * Writes a block of characters and attributes to the console.
     */
    void console_write_block(int x, int y, int width, int height, const int* chars, const int* attrs);

    /**
     * Reads a block of characters and attributes from the console.
     */
    void console_read_block(int x, int y, int width, int height, int* chars, int* attrs);

    /**
     * Toggles cursor visibility.
     */
    void console_set_cursor_visible(int visible);

    /**
     * Switches to/from the alternate screen buffer.
     */
    void console_use_alternate_buffer(int enable);
}

#endif // CONSOLE_NATIVE_H
