#ifndef TIME_NATIVE_H
#define TIME_NATIVE_H

/**
 * Declares monotonic timing functions.
 */
extern "C" {
    double time_stamp();
    long long time_nano_stamp();
}

#endif // TIME_NATIVE_H
