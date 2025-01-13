#pragma once

#include <stdint.h>

#if !defined(LOG_TRACE)
#define LOG_TRACE 0
#endif
#if !defined(LOG_DEBUG)
#define LOG_DEBUG 0
#endif

/**
  Implements RV32IM virtual machine

  registers - pointer to 31 32-bit registers. Can be NULL, then registers are
  zeroed at the start. program - raw binary RISC-V program program_len - length
  of the program

 */
int riscv_vm_run(uint8_t *registers, uint8_t *program, uint32_t program_len,
                 uint8_t *data, uint32_t data_len, uint32_t data_offset);
