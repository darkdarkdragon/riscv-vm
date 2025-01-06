#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "riscv-vm-optimized-1.h"

#define SYS_write 64

// 1 MiB, memory available inside VM
const int VM_MEMORY = 1048576;
// 32 registers, 32 bits each
const int REG_MEM_SIZE = 32 * 4;

// io devices
const uint32_t UART_OUT_REGISTER = 0x10000000;

const size_t alignment = 64;
const size_t work_mem_size = VM_MEMORY * 2; // size of memory to allocate

const int ERR_OUT_OF_MEM = 124;

// magic memory addres to communicate with the host
const uint32_t tohost = 0x1000;
const uint32_t fromhost = 0x1040;

int riscv_vm_main_loop(uint8_t *wmem, uint32_t program_len);

int riscv_vm_run_optimized_1(uint8_t *registers, uint8_t *program, uint32_t program_len)
{

   uint8_t *wmem = aligned_alloc(alignment, work_mem_size);
   if (wmem == NULL)
   {
      // Handle allocation failure
      perror("Memory allocation failed");
      return ERR_OUT_OF_MEM;
   }
   memset(wmem, 0, work_mem_size);
   if (registers)
   {
      memcpy(wmem, registers, REG_MEM_SIZE);
   }
   uint8_t *prog_start = wmem + REG_MEM_SIZE;
   memcpy(prog_start, program, program_len);

   int res = riscv_vm_main_loop(wmem, program_len);
   free(wmem);
   return res;
}





int riscv_vm_main_loop(uint8_t *wmem, uint32_t program_len) {

   return 0;
}
