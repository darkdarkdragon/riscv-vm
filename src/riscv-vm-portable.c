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

// io devices
const uint32_t UART_OUT_REGISTER = 0x10000000;

const size_t alignment = 64;
const size_t work_mem_size = VM_MEMORY * 2; // size of memory to allocate

const int ERR_OUT_OF_MEM = 124;

int riscv_vm_main_loop(uint8_t *wmem, uint32_t program_len);

int riscv_vm_run(uint8_t *registers, uint8_t *program, uint32_t program_len,
                 uint8_t *data, uint32_t data_len, uint32_t data_offset) {
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
  if (data) {
    if (data_offset + data_len > work_mem_size) {
      fprintf(stderr,
              "data placed outside of available memory (data start 0x%X end "
              "0x%X data len %d)\n",
              data_offset, data_offset + data_len, data_len);
      exit(11);
    }
    memcpy(prog_start + data_offset, data, data_len);
  }

  int res = riscv_vm_main_loop(wmem, program_len);
  free(wmem);
  return res;
}
// 0x10_00_00_00
void print_binary_8(uint8_t value);
void print_binary_32(uint32_t value);
void op_system(uint8_t *wmem, uint32_t instruction);
uint8_t op_branch(uint8_t *wmem, uint32_t instruction, uint8_t **pcp);
void op_jal(uint8_t *wmem, uint32_t instruction, uint8_t **pcp, uint32_t pc);
void op_jalr(uint8_t *wmem, uint32_t instruction, uint8_t **pcp, uint32_t pc);
void op_lui(uint8_t *wmem, uint32_t instruction);
void op_auipc(uint8_t *wmem, uint32_t instruction, uint32_t pc);
void op_int_imm_op(uint8_t *wmem, uint32_t instruction);
void op_int_op(uint8_t *wmem, uint32_t instruction);
void op_load(uint8_t *wmem, uint32_t instruction);
void op_store(uint8_t *wmem, uint32_t instruction);

void dbg_dump_registers_short(uint8_t *wmem);

int riscv_vm_main_loop(uint8_t *wmem, uint32_t program_len) {
  uint32_t pc = 0;
  volatile uint32_t *pcp = (uint32_t *)(wmem + REG_MEM_SIZE);
  uint32_t *program_end = pcp + program_len;
  uint8_t opcode = 0;
  uint32_t instruction = 0;
  printf("running instructions:\n");
  uint8_t going = 0;

  for (; pcp < program_end;) {
    instruction = *pcp;
    // printf(">>> pcp 0x%X inst %04X pe %X\n", pcp, instruction, program_end);
    // printf(">>> pcp 0x%X inst %04X pe %X\n", pcp, instruction, program_end);
    if (instruction == 0) {
      return 0;
    }
    opcode = instruction & 0x7F;
    pc = (uint32_t)((uint8_t *)pcp - (wmem + REG_MEM_SIZE));
    // printf("pc %02X inst %08X opcode %02X (%07b)\n", pc,instruction, opcode,
    // opcode);
    printf("---> pc 0x%02X inst %08X opcode 0x%02X\n", pc, instruction, opcode);
    dbg_dump_registers_short(wmem);
    print_binary_32(instruction);
    printf("addr %04X pc 0x%02X inst 0x%08X opcode 0x%02X (", pcp, pc,
           instruction, opcode);
    print_binary_8(opcode);
    printf(") - 8 bit %02X\n", instruction & 0xFF);
    switch (opcode) {
    case 0x73:
      op_system(wmem, instruction);
      break;
    case 0x63:
      if (op_branch(wmem, instruction, (uint8_t **)&pcp)) {
        continue;
      }
      break;
    case 0x6f:
      op_jal(wmem, instruction, (uint8_t **)&pcp, pc);
      continue;
    case 0x67:
      op_jalr(wmem, instruction, (uint8_t **)&pcp, pc);
      continue;
    case 0x37:
      op_lui(wmem, instruction);
      break;
    case 0x17:
      op_auipc(wmem, instruction, pc);
      break;
    case 0x13:
      op_int_imm_op(wmem, instruction);
      break;
    case 0x33:
      op_int_op(wmem, instruction);
      break;
    case 0x03:
      op_load(wmem, instruction);
      break;
    case 0x23:
      op_store(wmem, instruction);
      break;

    default:
      break;
    }
    // printf("------------------\n");
    pcp++;
  }
  printf("!! loop ENDED\n");
  return 0;
}

void op_store(uint8_t *wmem, uint32_t instruction) {
  uint32_t *reg = (uint32_t *)wmem;
  const uint8_t funct3 = (instruction >> 12) & 0x7;
  const uint8_t rs1 = (instruction >> 15) & 0x1F;
  const uint8_t rs2 = (instruction >> 20) & 0x1F;
  // const uint32_t imm = ((instruction >> 7) & 0x1F) | (instruction & (1 <<
  // 31)) |
  //                      ((instruction >> 20) & 0x7E0);
  // const int32_t immi = (int32_t)imm;
  const uint32_t imm =
      (instruction & 0xFE000000) | ((instruction & 0xF80) << 13);
  const int32_t immi = ((int32_t)imm) >> 19;

  const uint32_t addr = reg[rs1] + immi;

  printf("store rs1 %d rs2 %d width %d immi %d addr 0x%04X\n", rs1, rs2, funct3,
         immi, addr);
  if (addr == UART_OUT_REGISTER) {
    uint8_t byte;
    uint16_t half;
    uint32_t word;
    switch (funct3) {
    case 0: // sb
      byte = (uint8_t)reg[rs2];
      fprintf(stderr, "UART OUT: %c\n", byte);
      break;
    case 1: // sh
      half = (uint16_t)reg[rs2];
      fprintf(stderr, "%d", half);
      break;
    case 2: // sw
      word = reg[rs2];
      fprintf(stderr, "%d", word);
      break;
    default:
      break;
    }
    return;
  }

  switch (funct3) {
  case 0: // sb
    wmem[REG_MEM_SIZE + addr] = (uint8_t)reg[rs2];
    break;
  case 1: // sh
    wmem[REG_MEM_SIZE + addr] = (uint16_t)reg[rs2];
    break;
  case 2: // sw
    wmem[REG_MEM_SIZE + addr] = reg[rs2];
    break;
  default:
    break;
  }
}

void op_load(uint8_t *wmem, uint32_t instruction) {
  uint32_t *reg = (uint32_t *)wmem;
  const uint8_t funct3 = (instruction >> 12) & 0x7;
  const uint8_t rs1 = (instruction >> 15) & 0x1F;
  const uint8_t rd = (instruction >> 7) & 0x1F;
  if (rd == 0) {
    return;
  }
  //   const uint32_t imm = instruction >> 20;
  // const uint32_t imm = ((instruction >> 20) & 0x7FF) | (instruction & (1 <<
  // 31)); const int32_t immi = (int32_t)imm;

  const uint32_t imm = (instruction & 0xFFF00000);
  const int32_t immi = ((int32_t)imm) >> 20;
  uint32_t addr = reg[rs1] + immi;
  switch (funct3) {
  case 0: // lb
    reg[rd] = (int8_t)wmem[REG_MEM_SIZE + addr];
    break;
  case 1: // lh
    reg[rd] = (int16_t)wmem[REG_MEM_SIZE + addr];
    break;
  case 2: // lw
    reg[rd] = wmem[REG_MEM_SIZE + addr];
    break;
  case 4: // lbu
    reg[rd] = wmem[REG_MEM_SIZE + addr];
    break;
  case 5: // lhu
    reg[rd] = wmem[REG_MEM_SIZE + addr];
    break;
  default:
    break;
  }

  printf("load width %d rs1 %d rd %d imm %d addr 0x%02X rd_new_val 0x%04X\n",
         funct3, rs1, rd, immi, addr, reg[rd]);
}

void op_int_op(uint8_t *wmem, uint32_t instruction) {
  uint32_t *reg = (uint32_t *)wmem;
  const uint8_t rd = (instruction >> 7) & 0x1F;
  if (rd == 0) {
    return;
  }
  const uint8_t funct3 = (instruction >> 12) & 0x7;
  const uint8_t funct7 = (instruction >> 25);
  const uint8_t rs1 = (instruction >> 15) & 0x1F;
  const uint8_t rs2 = (instruction >> 20) & 0x1F;
  switch (funct3) {
  case 0: // add/sub
    if (funct7 == 0) {
      reg[rd] = reg[rs1] + reg[rs2];
    } else {
      reg[rd] = reg[rs1] - reg[rs2];
    }
    break;
  case 1: // sll
    reg[rd] = reg[rs1] << (reg[rs2] & 0x1F);
    break;
  case 2: // slt
    reg[rd] = (int32_t)reg[rs1] < (int32_t)reg[rs2] ? 1 : 0;
    break;
  case 3: // sltu
    reg[rd] = reg[rs1] < reg[rs2] ? 1 : 0;
    break;
  case 4: // xor
    reg[rd] = reg[rs1] ^ reg[rs2];
    break;
  case 5: // srl and sra
    if (funct7 == 0) {
      // srl
      reg[rd] = reg[rs1] >> (reg[rs2] & 0x1F);
    } else {
      // sra
      reg[rd] = (int32_t)reg[rs1] >> (reg[rs2] & 0x1F);
    }
  case 6: // or
    reg[rd] = reg[rs1] | reg[rs2];
    break;
  case 7: // and
    reg[rd] = reg[rs1] & reg[rs2];
    break;
  default:
    break;
  }
  printf("int reg-reg op %d rd %d rs1 %d rs2 %d rd_new_val 0x%04X\n", funct3,
         rd, rs1, rs2, reg[rd]);
}

void op_int_imm_op(uint8_t *wmem, uint32_t instruction) {
  uint32_t *reg = (uint32_t *)wmem;
  uint8_t shift;
  const uint8_t funct3 = (instruction >> 12) & 0x7;
  const uint8_t rs1 = (instruction >> 15) & 0x1F;
  const uint8_t rd = (instruction >> 7) & 0x1F;
  // const uint32_t imm =
  //     ((instruction >> 20) & 0x7FF) | (instruction & (1 << 31));
  // const int32_t immi = (int32_t)imm;

  if (rd != 0) {
    const uint32_t imm = (instruction >> 20) & 0xFFF;
    const int32_t immi = (int32_t)(imm << 20) >> 20;
    switch (funct3) {
    case 0: // addi
      reg[rd] = reg[rs1] + immi;
      break;
    case 2: // slti
      reg[rd] = (int32_t)reg[rs1] < immi ? 1 : 0;
      break;
    case 3: // sltiu
      reg[rd] = reg[rs1] < imm ? 1 : 0;
      break;
    case 4: // xori
      reg[rd] = reg[rs1] ^ immi;
      break;
    case 6: // ori
      reg[rd] = reg[rs1] | immi;
      break;
    case 7: // andi
      reg[rd] = reg[rs1] & immi;
      break;
    case 1: // slli
      shift = (instruction >> 20) & 0x1f;
      reg[rd] = reg[rs1] << shift;
      break;
    case 5: // srli and srai
      shift = (instruction >> 20) & 0x1f;
      if ((instruction >> 30) & 1) {
        // srai
        reg[rd] = (int32_t)reg[rs1] >> immi;
      } else {
        // srli
        reg[rd] = reg[rs1] >> immi;
      }
      break;
    default:
      break;
    }
    printf("int op %d rd %d rs %d imm 0x%02X (%d) rd_new_val 0x%04X\n", funct3,
           rd, rs1, immi, immi, reg[rd]);
  }
}

void op_auipc(uint8_t *wmem, uint32_t instruction, uint32_t pc) {
  uint32_t *reg = (uint32_t *)wmem;
  const uint8_t rd = (instruction >> 7) & 0x1F;
  const uint32_t imm = instruction & 0xFFFFF000;
  const uint32_t new_rd_val = pc + imm;
  printf("auipc rd %02d imm 0x%04X pc 0x%02X new_rd_val 0x%02X\n", rd, imm, pc,
         new_rd_val);
  if (rd != 0) {
    reg[rd] = new_rd_val;
  }
}

void op_lui(uint8_t *wmem, uint32_t instruction) {
  uint32_t *reg = (uint32_t *)wmem;
  const uint8_t rd = (instruction >> 7) & 0x1F;
  const uint32_t imm = instruction & 0xFFFFF000;
  printf("lui rd %02d imm 0x%04X\n", rd, imm);
  if (rd != 0) {
    reg[rd] = imm;
  }
}

void op_jal(uint8_t *wmem, uint32_t instruction, uint8_t **pcp, uint32_t pc) {
  uint32_t *reg = (uint32_t *)wmem;
  const uint32_t next_pc = pc + 4;

  uint8_t rd = (instruction >> 7) & 0x1F;

  const uint32_t imm =
      ((instruction << 11) & 0x7F800000) | ((instruction << 2) & (1 << 22)) |
      ((instruction >> 9) & 0x3FF000) | (instruction & (1 << 31));
  int32_t immi = ((int32_t)imm) >> 11;
  if (rd) {
    reg[rd] = next_pc;
  }
  *pcp += immi;
  printf("jal rd %02d imm 0x%04X (%d) next_pc 0x%02X\n", rd, imm, immi,
         next_pc);
}

void op_jalr(uint8_t *wmem, uint32_t instruction, uint8_t **pcp, uint32_t pc) {
  uint32_t *reg = (uint32_t *)wmem;
  const uint32_t next_pc = pc + 4;

  const uint8_t funct3 = (instruction >> 12) & 0x7;
  const uint8_t rs1 = (instruction >> 15) & 0x1F;
  const uint8_t rd = (instruction >> 7) & 0x1F;
  const uint32_t imm = (instruction & 0xFFF00000);
  const int32_t immi = ((int32_t)imm) >> 20;
  const uint32_t addr = (reg[rs1] + immi) & 0xFFFFFFFE;

  if (rd) {
    reg[rd] = next_pc;
  }
  *pcp = wmem + REG_MEM_SIZE + addr;
  printf("jalr rd %02d rs1 %d addr %02X immi %d next_pc 0x%02X\n", rd, rs1,
         addr, immi, next_pc);
  if (addr & 0x3) {
    fprintf(stderr, "!! addr not aligned %X\n", addr);
    exit(1);
  }
}

uint8_t op_branch(uint8_t *wmem, uint32_t instruction, uint8_t **pcp) {
  print_binary_32(instruction);
  const uint8_t funct3 = (instruction >> 12) & 0x7;
  const uint8_t rs1 = (instruction >> 15) & 0x1F;
  const uint8_t rs2 = (instruction >> 20) & 0x1F;
  // const uint32_t imm =
  //     ((instruction >> 7) & 0x1E) | ((instruction >> 20) & 0x7E0) |
  //     ((instruction << 4) & 0x800) | (instruction & 0x80000000);
  // const int32_t imms = (int32_t)imm;
  const uint32_t imm =
      (instruction & 0x80000000) | ((instruction & (1 << 7)) << 23) |
      ((instruction & 0x7E000000) >> 1) | ((instruction & 0xF00) << 12);

  const int32_t imms = ((int32_t)imm) >> 19;
  const uint32_t *reg = (uint32_t *)wmem;
  uint8_t is_taken = 0;
  switch (funct3) {
  case 0: // beq
    is_taken = reg[rs1] == reg[rs2];
    break;
  case 1: // bne
    is_taken = reg[rs1] != reg[rs2];
    break;
  case 4: // blt
    is_taken = (int32_t)reg[rs1] < (int32_t)reg[rs2];
    break;
  case 5: // bge
    is_taken = (int32_t)reg[rs1] >= (int32_t)reg[rs2];
    break;
  case 6: // bltu
    is_taken = reg[rs1] < reg[rs2];
    break;
  case 7: // bgeu
    is_taken = reg[rs1] >= reg[rs2];
    break;
  }
  printf("branch funct3 %01X rs1 %d rs2 %d imms %d is_taken %d\n", funct3, rs1,
         rs2, imms, is_taken);
  *pcp += is_taken ? imms : 0;
  return is_taken;
}

void op_ecall(uint8_t *wmem, uint32_t instruction) {
  uint32_t *reg = (uint32_t *)wmem;
  uint32_t gp = reg[3];
  uint32_t a0 = reg[10]; // argument
  uint32_t a7 = reg[17]; // function
  uint8_t is_test;
  uint32_t exit_code;
  switch (a7) {
  case 93:
    // exit
    is_test = a0 & 1;
    exit_code = a0 >> 1;
    if (is_test && a0) {
      fprintf(stderr, "*** FAILED *** (tohost = %d)\n", exit_code);
    }
    printf("exit code %d\n", exit_code);
    exit(a0);
    break;
  case 63:
    // read
    // printf("read\n");
    // reg[a0] = getchar();
    break;
  case 64:
    // write
    printf("write %c\n", a0);
    putchar(a0);
    break;
  default:
    break;
  }

  printf("ecall\n");
}

void op_ebreak(uint32_t instruction) { printf("ebreak\n"); }

void op_system(uint8_t *wmem, uint32_t instruction) {
  const uint8_t funct3 = (instruction >> 12) & 0x7;
  const uint8_t funct7 = (instruction >> 25);
  printf("system funct3 %02X funct7 %02X\n", funct3, funct7);
  if (funct3 == 0x0) {
    // printf("ecall funct7 %02X\n", funct7);
    switch (funct7) {
    case 0x0:
      // ecall
      op_ecall(wmem, instruction);
      break;
    case 0x1:
      // ebreak
      op_ebreak(instruction);
      break;
    case 0x8:
      // wfi
      printf("wfi\n");
      exit(0);
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
    switch (csr) {
    case 0xF14: // mhartid (Hardware thread ID.)
    {
      //   uint32_t *reg = (uint32_t *)(wmem + REG_MEM_SIZE);
      if (rd) {
        ((uint32_t *)wmem)[rd] = 0;
      }
    } break;
    default:
      break;
    }
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

void dbg_dump_registers_short(uint8_t *wmem) {
  uint32_t *reg = (uint32_t *)wmem;
  printf("registers: ");
  for (int i = 0; i < 32; i++) {
    uint32_t val = reg[i];
    if (val) {
      printf("reg %02d 0x%04X ", i, val);
    }
  }
  printf("\n");
}