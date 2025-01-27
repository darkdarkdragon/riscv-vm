#pragma once

#include <stdint.h>
#include <stdlib.h>

#define SYS_write 64
#define SYS_print_mem_access 2048
#define INT_MIN_HEX 0x80000000

// 32 registers, 32 bits each
static const int REG_MEM_SIZE = 32 * 4;

// io devices
static const uint32_t UART_OUT_REGISTER = 0x10000000;

static const size_t alignment = 64;

static const int ERR_OUT_OF_MEM = 124;
static const int ERR_UNIMPLEMENTED_OPCODE = 123;
static const int ERR_INVALID_MEMORY_ACCESS = 122;
static const int ERR_MISALIGNED_MEMORY_ACCESS = 121;
static const int ERR_UNIMPLEMENTED_MAGIC_SYSCALL = 120;
static const int ERR_EBREAK = 100;
static const int ERR_WFI = 99;

// magic memory addres to communicate with the host
static const uint32_t tohost = 0x1000;
static const uint32_t fromhost = 0x1040;


static const char *reg_func_name[] __attribute__((unused)) = {
    "", "csrrw", "csrrs", "csrrc", "csrrwi", "csrrsi", "csrrci"};

void dump_registers(uint8_t *registers) __attribute__((unused));
