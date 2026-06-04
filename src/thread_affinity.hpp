#pragma once

#ifdef __linux__

#include <pthread.h>
#include <sched.h>

inline bool pin_current_thread_to_cpu(int cpu) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);

    return pthread_setaffinity_np(pthread_self(), sizeof(set), &set) == 0;
}

#else

inline bool pin_current_thread_to_cpu(int) {
    return false;
}

#endif
