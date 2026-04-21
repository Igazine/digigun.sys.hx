#ifndef RT_NATIVE_H
#define RT_NATIVE_H

/**
 * Native real-time functions.
 */
extern "C" {
    int rt_mlockall(int current, int future);
    int rt_munlockall();
}

#endif // RT_NATIVE_H
