
#include "riscv-vm-common.h"

void dump_registers(uint8_t *registers __attribute__((unused))) {
#if USE_PRINT
  uint32_t *reg = (uint32_t *)registers;
  printf("registers:\n");
  for (int i = 0; i < 32; i++) {
    uint32_t val = reg[i];
    printf("%02d 0x%04X ", i, val);
  }
  printf("\n");
#endif
}
