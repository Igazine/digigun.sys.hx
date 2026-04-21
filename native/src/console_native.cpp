#include "console_native.h"
#include <cstdio>
#include <vector>

#ifdef _WIN32
#include <windows.h>

static DWORD original_mode = 0;
static bool mode_saved = false;

extern "C" {

int console_set_raw_mode(int enabled) {
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    if (hInput == INVALID_HANDLE_VALUE) return 0;
    if (!mode_saved) { GetConsoleMode(hInput, &original_mode); mode_saved = true; }
    if (enabled) {
        DWORD raw_mode = original_mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
        return SetConsoleMode(hInput, raw_mode) ? 1 : 0;
    } else {
        return SetConsoleMode(hInput, original_mode) ? 1 : 0;
    }
}

void console_get_size(int* width, int* height) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        *width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        *height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    } else {
        *width = *height = -1;
    }
}

void console_write_block(int x, int y, int width, int height, const int* chars, const int* attrs) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    std::vector<CHAR_INFO> buffer(width * height);
    for (int i = 0; i < width * height; i++) {
        buffer[i].Char.AsciiChar = (char)chars[i];
        buffer[i].Attributes = (WORD)attrs[i];
    }
    COORD bSize = {(SHORT)width, (SHORT)height};
    COORD bPos = {0, 0};
    SMALL_RECT wRegion = {(SHORT)x, (SHORT)y, (SHORT)(x + width - 1), (SHORT)(y + height - 1)};
    WriteConsoleOutputA(hOut, buffer.data(), bSize, bPos, &wRegion);
}

void console_read_block(int x, int y, int width, int height, int* chars, int* attrs) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    std::vector<CHAR_INFO> buffer(width * height);
    COORD bSize = {(SHORT)width, (SHORT)height};
    COORD bPos = {0, 0};
    SMALL_RECT rRegion = {(SHORT)x, (SHORT)y, (SHORT)(x + width - 1), (SHORT)(y + height - 1)};
    if (ReadConsoleOutputA(hOut, buffer.data(), bSize, bPos, &rRegion)) {
        for (int i = 0; i < width * height; i++) {
            chars[i] = (unsigned char)buffer[i].Char.AsciiChar;
            attrs[i] = buffer[i].Attributes;
        }
    }
}

void console_set_cursor_visible(int visible) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    GetConsoleCursorInfo(hOut, &info);
    info.bVisible = visible ? TRUE : FALSE;
    SetConsoleCursorInfo(hOut, &info);
}

void console_use_alternate_buffer(int enable) {
    // Windows doesn't have a direct "alternate buffer" in the same way, 
    // but we can use CreateConsoleScreenBuffer. For simplicity, we'll use ANSI sequences.
    printf(enable ? "\x1b[?1049h" : "\x1b[?1049l");
}

} // extern "C"

#else
// POSIX Implementation
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

static struct termios original_termios;
static bool mode_saved = false;

extern "C" {

int console_set_raw_mode(int enabled) {
    if (!mode_saved) { if (tcgetattr(STDIN_FILENO, &original_termios) == -1) return 0; mode_saved = true; }
    if (enabled) {
        struct termios raw = original_termios;
        raw.c_lflag &= ~(ICANON | ECHO | ISIG);
        raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0;
        return tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0 ? 1 : 0;
    } else {
        return tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios) == 0 ? 1 : 0;
    }
}

void console_get_size(int* width, int* height) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != -1) { *width = ws.ws_col; *height = ws.ws_row; }
    else { *width = *height = -1; }
}

void console_write_block(int x, int y, int width, int height, const int* chars, const int* attrs) {
    // POSIX uses ANSI escape sequences for TUI
    for (int row = 0; row < height; row++) {
        printf("\x1b[%d;%dH", y + row + 1, x + 1);
        for (int col = 0; col < width; col++) {
            int idx = row * width + col;
            // Simplified: we'll handle basic 16-color attributes for now
            // Higher bits could handle 256 or true color
            int attr = attrs[idx];
            printf("\x1b[%dm%c", attr, (char)chars[idx]);
        }
    }
    printf("\x1b[0m"); // reset
    fflush(stdout);
}

void console_read_block(int x, int y, int width, int height, int* chars, int* attrs) {
    // POSIX cannot easily read from screen buffer without complex logic (ncurses).
    // Usually TUIs maintain a local buffer in memory.
}

void console_set_cursor_visible(int visible) {
    printf(visible ? "\x1b[?25h" : "\x1b[?25l");
    fflush(stdout);
}

void console_use_alternate_buffer(int enable) {
    printf(enable ? "\x1b[?1049h" : "\x1b[?1049l");
    fflush(stdout);
}

} // extern "C"
#endif
