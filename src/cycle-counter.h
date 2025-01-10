#pragma once

#include <stdint.h>

#if defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>
static inline uint64_t get_cycles(void) {
    // Notes:
    // - RDTSC reads the time stamp counter
    // - _mm_lfence() prevents out-of-order execution
    // - Newer Intel CPUs might need RDTSCP instead of RDTSC for better accuracy
    //_mm_lfence();  // Serializing instruction
    return __rdtsc();
}
static inline void init_counter(void) {
}

#elif defined(__aarch64__)
    #if defined(__APPLE__)
    #include <mach/mach_time.h>
    static inline uint64_t get_cycles(void) {
        // On Apple Silicon, we use mach_absolute_time instead of CPU cycles
        return mach_absolute_time();
    }
    static inline void init_counter(void) {
    }
    
    // Convert mach_absolute_time units to nanoseconds
    static inline double get_cycles_ns(uint64_t cycles) {
        static double timebase = 0.0;
        if (timebase == 0.0) {
            mach_timebase_info_data_t timebase_info;
            mach_timebase_info(&timebase_info);
            timebase = (double)timebase_info.numer / (double)timebase_info.denom;
        }
        return (double)cycles * timebase;
    }
    
    #else  // Other ARM64 platforms (Linux, etc.)
    static inline uint64_t get_cycles(void) {
        uint64_t val;
        // Read PMCCNTR_EL0 (Performance Monitors Cycle Count Register)
        asm volatile("mrs %0, pmccntr_el0" : "=r" (val));
        return val;
    }
    // Function to enable cycle counter on ARM
    static inline void init_counter(void) {
        // Enable user mode access to cycle counter
        asm volatile("msr pmuserenr_el0, %0" : : "r"(1UL));
        // Enable cycle counter
        asm volatile("msr pmcntenset_el0, %0" : : "r"(1UL << 31));
        // Reset cycle counter
        asm volatile("msr pmccfiltr_el0, %0" : : "r"(0));
    }
    #endif

#else
#error "Architecture not supported"
#endif
