
#include <stdint.h>

#define LOG_TRACE 0
#define LOG_DEBUG 0


/**
  Implements RV32IM virtual machine

  registers - pointer to 31 32-bit registers. Can be NULL, then registers are
  zeroed at the start. program - raw binary RISC-V program program_len - length
  of the program

 */
int riscv_vm_run_optimized_1(uint8_t *registers, uint8_t *program, uint32_t program_len);
