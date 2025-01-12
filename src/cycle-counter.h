#pragma once

#include <stdint.h>

#if defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>

static struct timespec ts;
static uint64_t start_time;

static inline uint64_t get_cycles(void) {
  // Notes:
  // - RDTSC reads the time stamp counter
  // - _mm_lfence() prevents out-of-order execution
  // - Newer Intel CPUs might need RDTSCP instead of RDTSC for better accuracy
  //_mm_lfence();  // Serializing instruction
  //   return __rdtsc();
  clock_gettime(CLOCK_MONOTONIC, &ts);
  uint64_t nanoseconds = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
  return nanoseconds - start_time;
}

static inline void init_counter(void) {
  clock_gettime(CLOCK_MONOTONIC, &ts);
  uint64_t nanoseconds = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
  start_time = nanoseconds;
}

#elif defined(__aarch64__)
#if defined(__APPLE__)
#include <_time.h>
#include <mach/mach_time.h>
#include <stdio.h>

static mach_timebase_info_data_t _clock_timebase;
static uint64_t __start_time;

static inline uint64_t get_cycles(void) {
  // On Apple Silicon, we use mach_absolute_time instead of CPU cycles
  // return mach_absolute_time();
  return clock_gettime_nsec_np(CLOCK_UPTIME_RAW) - __start_time;
  // nanos = (mach_time * _clock_timebase.numer)
}

static inline void init_counter(void) {
   __start_time = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
  // double nanos = (mach_time * _clock_timebase.numer) / _clock_timebase.denom;
//   mach_timebase_info(&_clock_timebase);
//   uint64_t mach_time = mach_absolute_time();
//   uint64_t nano_time = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
//   printf("------> time base number %d denom %d nano_time %llu, mach time %llu\n",
        //  _clock_timebase.numer, _clock_timebase.denom, nano_time, mach_time);
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

#else // Other ARM64 platforms (Linux, etc.)
static inline uint64_t get_cycles(void) {
  uint64_t val;
  // Read PMCCNTR_EL0 (Performance Monitors Cycle Count Register)
  asm volatile("mrs %0, pmccntr_el0" : "=r"(val));
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
