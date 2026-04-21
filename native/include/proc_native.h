#ifndef PROC_NATIVE_H
#define PROC_NATIVE_H

/**
 * Declares CPU affinity and scheduling system calls.
 */
extern "C" {
    /**
     * Sets CPU affinity mask.
     * @return 1 on success, 0 on failure.
     */
    int proc_set_affinity(int mask);

    /**
     * Gets CPU affinity mask.
     * @return The mask, or -1 on failure.
     */
    int proc_get_affinity();

    /**
     * Sets process priority class.
     * @param priority_class 0: Idle, 1: BelowNormal, 2: Normal, 3: AboveNormal, 4: High, 5: Realtime
     * @return 1 on success, 0 on failure.
     */
    int proc_set_priority(int priority_class);
}

#endif // PROC_NATIVE_H
