#pragma once

#include <stdint.h>

#define LOG_TRACE 0
#define LOG_DEBUG 0
#define USE_PRINT 1
#define DISALLOW_MISALIGNED 0
#define EMULATE_UART_OUT 1
// used by benchmarks in https://github.com/riscv-software-src/riscv-tests
#define USE_TOHOST_SYSCALL 1
#define PRINT_REGISTERS 0

extern char *op_names[128];

/**
  Implements RV32IM virtual machine

  registers - pointer to 31 32-bit registers. Can be NULL, then registers are
  zeroed at the start. program - raw binary RISC-V program program_len - length
  of the program

 */
int riscv_vm_run_optimized_1(uint8_t *registers, uint8_t *program, uint32_t program_len);

int riscv_vm_run_optimized_2(uint8_t *registers, uint8_t *program, uint32_t program_len);

int riscv_vm_run_optimized_3(uint8_t *registers, uint8_t *program, uint32_t program_len);

int riscv_vm_run_optimized_4(uint8_t *registers, uint8_t *program, uint32_t program_len);

uint32_t syscall_handler(uint32_t syscall_number, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5, uint32_t arg6, uint32_t arg7, void* wmem);
