#pragma once

#include <stdint.h>

#define SYS_write 64
#define INT_MIN_HEX 0x80000000

// 32 registers, 32 bits each
static const int REG_MEM_SIZE = 32 * 4;


static const char *reg_func_name[] __attribute__((unused)) = {
    "", "csrrw", "csrrs", "csrrc", "csrrwi", "csrrsi", "csrrci"};

void dump_registers(uint8_t *registers) __attribute__((unused));
