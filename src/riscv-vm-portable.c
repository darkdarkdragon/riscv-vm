#include <_stdio.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "riscv-vm-portable.h"

// 1 MiB, memory available inside VM
const int VM_MEMORY = 1048576;
// 32 registers, 32 bits each
const int REG_MEM_SIZE = 32 * 4;

const size_t alignment = 64;
const size_t work_mem_size = VM_MEMORY * 2; // size of memory to allocate

const int ERR_OUT_OF_MEM = 124;

int riscv_vm_main_loop(uint8_t *wmem, uint32_t program_len);

int riscv_vm_run(uint8_t *registers, uint8_t *program, uint32_t program_len) {
  uint8_t *wmem = aligned_alloc(alignment, work_mem_size);
  if (wmem == NULL) {
    // Handle allocation failure
    perror("Memory allocation failed");
    return ERR_OUT_OF_MEM;
  }
  memset(wmem, 0, work_mem_size);
  if (registers) {
    memcpy(wmem, registers, REG_MEM_SIZE);
  }
  uint8_t *prog_start = wmem + REG_MEM_SIZE;
  memcpy(prog_start, program, program_len);
  int res = riscv_vm_main_loop(wmem, program_len);
  free(wmem);
  return res;
}
// 0x10_00_00_00
void print_binary_8(uint8_t value);
void op_system(uint32_t instruction);
void op_beq(uint32_t instruction);
void op_lui(uint32_t instruction);
void op_auipc(uint32_t instruction);
void op_addi(uint32_t instruction);
void op_lx(uint32_t instruction);
void op_sx(uint32_t instruction);

int riscv_vm_main_loop(uint8_t *wmem, uint32_t program_len) {
  uint32_t pc = 0;
  uint32_t *pcp = (uint32_t *)(wmem + REG_MEM_SIZE);
  uint32_t *program_end = pcp + program_len;
  uint8_t opcode = 0;
  uint32_t instruction = 0;
  printf("running instructions:\n");

  for (; pcp < program_end; pcp++, pc += 4) {
    instruction = *pcp;
    opcode = instruction & 0x7F;
    // printf("pc %02X inst %08X opcode %02X (%07b)\n", pc,instruction, opcode,
    // opcode);
    printf("pc %02X inst %08X opcode %02X (", pc, instruction, opcode);
    print_binary_8(opcode);
    printf(") - 8 bit %02X\n", instruction & 0xFF);
    switch (opcode) {
    case 0x73:
      op_system(instruction);
      break;
    case 0x63:
      op_beq(instruction);
      break;
    case 0x37:
      op_lui(instruction);
      break;
    case 0x17:
      op_auipc(instruction);
      break;
    case 0x13:
      op_addi(instruction);
      break;
    case 0x03:
      op_lx(instruction);
      break;
    case 0x23:
      op_sx(instruction);
      break;

    default:
      break;
    }
  }
  return 0;
}

void op_sx(uint32_t instruction) { printf("sx\n"); }

void op_lx(uint32_t instruction) { printf("lb\n"); }

void op_addi(uint32_t instruction) { printf("addi\n"); }

void op_auipc(uint32_t instruction) { printf("auipc\n"); }

void op_lui(uint32_t instruction) { printf("lui\n"); }

void op_beq(uint32_t instruction) { printf("beq\n"); }

void op_ecall(uint32_t instruction) { printf("ecall\n"); }

void op_ebreak(uint32_t instruction) { printf("ebreak\n"); }

void op_system(uint32_t instruction) {
  const uint8_t funct3 = (instruction >> 12) & 0x7;
  printf("ecall funct3 %02X\n", funct3);
  if (funct3 == 0x0) {
    const uint8_t funct7 = (instruction >> 20);
    // printf("ecall funct7 %02X\n", funct7);
    switch (funct7) {
    case 0x0:
      // ecall
      op_ecall(instruction);
      break;
    case 0x18:
      // ebreak
      op_ebreak(instruction);
      break;
    default:
      break;
    }
    return;
  }
  const uint8_t rd = (instruction >> 7) & 0x1F;
  const uint8_t rs1 = (instruction >> 15) & 0x1F;
  // const uint8_t rs2 = (instruction >> 20) & 0x1F;
  const uint32_t csr = (instruction >> 20) & 0xFFF;
  printf("rd %02d rs1 %02d csr %03X\n", rd, rs1, csr);

  // csr functions
  switch (funct3) {
  case 0x1:
    // csrrw
    break;
  case 0x2:
    // csrrs
    break;
  case 0x3:
    // csrrc
    break;
  case 0x5:
    // csrrwi
    break;
  case 0x6:
    // csrrsi
    break;
  case 0x7:
    // csrrci
    break;
  default:
    break;
  }
}

void print_binary_8(uint8_t value) {
  // 01110011
  for (int i = 7; i >= 0; i--) {
    printf("%c", (value & (1 << i)) ? '1' : '0');
    if (i % 4 == 0) { // Add a space every 4 bits for readability
      printf(" ");
    }
  }
  // printf("\n");
}

void print_binary_32(uint32_t value) {
  // Print 32 bits (assuming 4-byte integer)
  for (int i = 31; i >= 0; i--) {
    printf("%c", (value & (1 << i)) ? '1' : '0');
    if (i % 4 == 0) { // Add a space every 4 bits for readability
      printf(" ");
    }
  }
  printf("\n");
}
