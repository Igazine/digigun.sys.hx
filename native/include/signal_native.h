#ifndef SIGNAL_NATIVE_H
#define SIGNAL_NATIVE_H

/**
 * Native signal handling functions.
 */
extern "C" {
    int signal_trap(int signo, void* callback);
    int signal_raise(int signo);
    int signal_get_value(const char* name);
}

#endif // SIGNAL_NATIVE_H
