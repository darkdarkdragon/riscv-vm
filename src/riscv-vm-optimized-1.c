#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "riscv-vm-optimized-1.h"

#define SYS_write 64
#define INT_MIN_HEX 0x80000000

// 1 MiB, memory available inside VM
static const int VM_MEMORY = 1048576;
// 32 registers, 32 bits each
static const int REG_MEM_SIZE = 32 * 4;

// io devices
static const uint32_t UART_OUT_REGISTER = 0x10000000;

static const size_t alignment = 64;
static const size_t work_mem_size = VM_MEMORY * 2; // size of memory to allocate

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

static void dump_registers(uint8_t *registers);
static void dbg_dump_registers_short(uint8_t *wmem);

int riscv_vm_main_loop(uint8_t *initial_registers, uint8_t *wmem,
                       uint32_t program_len, uint32_t *pcp_out);

int riscv_vm_run_optimized_1(uint8_t *registers, uint8_t *program,
                             uint32_t program_len) {

  uint8_t *wmem = aligned_alloc(alignment, work_mem_size);
  if (wmem == NULL) {
    // Handle allocation failure
#if USE_PRINT
    fprintf(stderr, "Memory allocation failed\n");
#endif
    return ERR_OUT_OF_MEM;
  }
  memset(wmem, 0, work_mem_size);
  memcpy(wmem, program, program_len);
  uint32_t pcp = 0;
  if (!registers) {
    registers = malloc(REG_MEM_SIZE);
    if (registers == NULL) {
      // Handle allocation failure
#if USE_PRINT
      fprintf(stderr, "Memory allocation failed\n");
#endif
      return ERR_OUT_OF_MEM;
    }
    memset(registers, 0, REG_MEM_SIZE);
  }
  dump_registers(registers);
  int res = riscv_vm_main_loop(registers, wmem, program_len, &pcp);
  dump_registers(registers);
#if USE_PRINT
  printf("Final PC: %04X\n", pcp);
#endif
  free(wmem);
  return res;
}

#define GET_RD(inst) ((inst) >> 7) & 0x1F
#define GET_RS1(inst) ((inst) >> 15) & 0x1F
#define GET_RS2(inst) ((inst) >> 20) & 0x1F
#define GET_FUNCT3(inst) ((inst) >> 12) & 0x7
#define GET_FUNCT7(inst) ((inst) >> 25) & 0x7F
#define GET_IMM_I(inst) ((int32_t)(inst) >> 20)
#define GET_IMM_U(inst) ((inst) & 0xFFFFF000)
// #define GET_IMM_S(inst) ((int32_t)(((inst) >> 25) << 5) + ((inst) >> 7) &
// 0x1F)
#define GET_IMM_S(inst)                                                        \
  ((int32_t)((inst & 0xFE000000) | ((inst & 0xF80) << 13)) >> 20)

static inline int op_load(uint32_t *registers, uint8_t *wmem,
                          uint32_t instruction, uint32_t pc);
static inline void op_int_imm_op(uint32_t *registers, uint32_t instruction);
static inline void op_auipc(uint32_t *registers, uint32_t instruction,
                            uint32_t pc);
static inline int op_store(uint32_t *registers, uint8_t *wmem,
                           uint32_t instruction, uint32_t pc, int *exit_code);
static inline void op_int_op(uint32_t *registers, uint32_t instruction);
static inline void op_lui(uint32_t *registers, uint32_t instruction);
static inline uint8_t op_branch(uint32_t *registers, uint32_t instruction,
                                uint32_t *pc, int *exit_code);
static inline void op_jalr(uint32_t *registers, uint32_t instruction,
                           uint32_t *pc, int *exit_code);
static inline void op_jal(uint32_t *registers, uint32_t instruction,
                          uint32_t *pc, int *exit_code);
static inline int op_system(uint32_t *registers, uint32_t instruction,
                            int *ext_exit_code);

#define exit_loop(ec)                                                          \
  memcpy(initial_registers, registers, REG_MEM_SIZE);                          \
  *pcp_out = pc;                                                               \
  return ec;

int riscv_vm_main_loop(uint8_t *initial_registers, uint8_t *wmem,
                       uint32_t program_len, uint32_t *pcp_out) {
  uint32_t registers[32];
  uint8_t *program = wmem;
  uint32_t pc = 0;
  uint32_t instruction;
  int res = 0;
  memcpy(registers, initial_registers, REG_MEM_SIZE);
  while (res == 0) {
    instruction = *(uint32_t *)(program + pc);
#if LOG_TRACE
    printf("PC: 0x%04X inst 0x%08X ", pc, instruction);
    dbg_dump_registers_short(registers);
    //  printf("\n");
#endif
    switch (instruction & 0x7F) {
    case 0x03:
      if ((res = op_load(registers, program, instruction, pc))) {
        exit_loop(res);
      }
      break;
    case 0x0F:
      // misc-mem opcode, nop
      break;
    case 0x13:
      op_int_imm_op(registers, instruction);
      break;
    case 0x17:
      op_auipc(registers, instruction, pc);
      break;
    case 0x23:
      if (op_store(registers, program, instruction, pc, &res)) {
        exit_loop(res);
      }
      break;
    case 0x33:
      op_int_op(registers, instruction);
      break;
    case 0x37:
      op_lui(registers, instruction);
      break;
    case 0x63:
      if (op_branch(registers, instruction, &pc, &res)) {
        continue;
      }
      break;
    case 0x67:
      op_jalr(registers, instruction, &pc, &res);
      continue;
    case 0x6f:
      op_jal(registers, instruction, &pc, &res);
      continue;
    case 0x73:
      if (op_system(registers, instruction, &res)) {
        exit_loop(res);
      }
      break;
    default:
#if USE_PRINT
      fprintf(stderr, "Unimplemented or invalid opcode 0x%02X at PC 0x%02X\n",
              instruction & 0x7F, pc);
#endif
      exit_loop(ERR_UNIMPLEMENTED_OPCODE) break;
    }
    pc += 4;
  }
  exit_loop(res);
}

static inline int op_load(uint32_t *registers, uint8_t *wmem,
                          uint32_t instruction, uint32_t pc) {
  const uint8_t rd = GET_RD(instruction);
  if (rd == 0) {
    return 0;
  }
  uint32_t addr = registers[GET_RS1(instruction)] + GET_IMM_I(instruction);
#if LOG_TRACE
  printf("op_load addr 0x%x imm %d\n", addr, GET_IMM_I(instruction));
#endif
  // hack to account for code that uses absoulte addresses and start
  // virtual memory from 0x80000000
  addr &= 0x7FFFFFFF;
  if ((addr) >= work_mem_size) {
#if USE_PRINT
    fprintf(
        stderr,
        "load went beyond allocated memory pc %X (addr %0X) (instruction %X)\n",
        pc, addr, instruction);
#endif
    return ERR_INVALID_MEMORY_ACCESS;
  }
  switch (GET_FUNCT3(instruction)) {
  case 0: // lb
    registers[rd] = (int8_t)wmem[addr];
    break;
  case 1: // lh
#if DISALLOW_MISALIGNED
    if (addr & 1) {
#if USE_PRINT
      fprintf(stderr, "misaligned load at pc %X (addr %0X) (instruction %X)\n",
              pc, addr, instruction);
#endif
      return ERR_MISALIGNED_MEMORY_ACCESS;
    }
#endif
    registers[rd] = *(int16_t *)(wmem + addr);
    break;
  case 2: // lw
#if DISALLOW_MISALIGNED
    if (addr & 3) {
#if USE_PRINT
      fprintf(stderr,
              "misaligned load at pc 0x%X (addr 0x%0X) (instruction 0x%X)\n",
              pc, addr, instruction);
#endif
      return ERR_MISALIGNED_MEMORY_ACCESS;
    }
#endif
    registers[rd] = *(uint32_t *)(wmem + addr);
    break;
  case 4: // lbu
    registers[rd] = wmem[addr];
    break;
  case 5: // lhu
#if DISALLOW_MISALIGNED
    if (addr & 1) {
#if USE_PRINT
      fprintf(stderr,
              "misaligned load at pc 0x%X (addr 0x%0X) (instruction 0x%X)\n",
              pc, addr, instruction);
#endif
      return ERR_MISALIGNED_MEMORY_ACCESS;
    }
#endif
    registers[rd] = *(uint16_t *)(wmem + addr);
    break;
  default:
    break;
  }
  return 0;
}

static inline void op_int_imm_op(uint32_t *registers, uint32_t instruction) {
  const uint8_t rd = GET_RD(instruction);
  if (rd == 0) {
    return;
  }
  uint8_t shift;
  const uint8_t rs1 = GET_RS1(instruction);
  int32_t immi = GET_IMM_I(instruction);
  uint32_t v1 = registers[rs1];
  switch (GET_FUNCT3(instruction)) {
  case 0: // addi
    registers[rd] = v1 + immi;
    break;
  case 2: // slti
    registers[rd] = (int32_t)v1 < immi ? 1 : 0;
    break;
  case 3: // sltiu
    registers[rd] = v1 < ((uint32_t)immi) ? 1 : 0;
    break;
  case 4: // xori
    registers[rd] = v1 ^ immi;
    break;
  case 6: // ori
    registers[rd] = v1 | immi;
    break;
  case 7: // andi
    registers[rd] = v1 & immi;
    break;
  case 1: // slli
    shift = (instruction >> 20) & 0x1f;
    registers[rd] = v1 << shift;
    break;
  case 5: // srli and srai
    shift = (instruction >> 20) & 0x1f;
    if ((instruction >> 30) & 1) {
      // srai
      registers[rd] = (int32_t)v1 >> immi;
    } else {
      // srli
      registers[rd] = v1 >> immi;
    }
    break;
  default:
    break;
  }
}

static inline void op_auipc(uint32_t *registers, uint32_t instruction,
                            uint32_t pc) {
  const uint8_t rd = GET_RD(instruction);
  if (rd == 0) {
    return;
  }
  registers[rd] = pc + GET_IMM_U(instruction);
}

static inline int op_store(uint32_t *registers, uint8_t *wmem,
                           uint32_t instruction, uint32_t pc, int *exit_code) {
  const uint8_t funct3 = GET_FUNCT3(instruction);
  const uint32_t v2 = registers[GET_RS2(instruction)];
  uint32_t addr = registers[GET_RS1(instruction)] + GET_IMM_S(instruction);
#if LOG_TRACE
  printf("op_store addr 0x%x imm %d\n", addr, GET_IMM_S(instruction));
#endif

#if EMULATE_UART_OUT
  if (addr == UART_OUT_REGISTER) {
    uint8_t byte;
    uint16_t half;
    uint32_t word;
    switch (funct3) {
    case 0: // sb
      byte = (uint8_t)v2;
#if USE_PRINT
      printf("%c", byte);
#endif
      break;
    default:
      break;
    }
    return 0;
  }
#endif
  // hack to account for code that uses absoulte addresses and start
  // virtual memory from 0x80000000
  addr &= 0x7FFFFFFF;

  if ((addr) >= work_mem_size) {
#if USE_PRINT
    fprintf(stderr,
            "store went beyond allocated memory pc %X (addr %0X) (instruction "
            "%X)\n",
            pc, addr, instruction);
#endif
    *exit_code = ERR_INVALID_MEMORY_ACCESS;
    return 1;
  }
#if USE_TOHOST_SYSCALL
  // used by benchmarks in https://github.com/riscv-software-src/riscv-tests
  if (addr == tohost) {
    uint32_t from_virt = v2;
    uint8_t is_exit = from_virt & 1;
    if (is_exit) {
      *exit_code = from_virt >> 1;
      return 1;
    }
    from_virt = *(uint32_t *)(wmem + v2);
    if (from_virt == SYS_write) {
      uint32_t a0 = *(uint32_t *)(wmem + v2 + 8);
      uint32_t str_ptr = *(uint32_t *)(wmem + v2 + 16);
      uint32_t str_len = *(uint32_t *)(wmem + v2 + 24);
      for (int i = 0; i < str_len; i++) {
        printf("%c", wmem[str_ptr + i]);
      }
    } else {
#if USE_PRINT
      fprintf(stderr, "unimplemented magic syscall %d\n", from_virt);
#endif
      *exit_code = ERR_INVALID_MEMORY_ACCESS;
      return 1;
    }
    wmem[fromhost] = 1;
    return 0;
  }
#endif

  switch (funct3) {
  case 0: // sb
    wmem[addr] = (uint8_t)v2;
    break;
  case 1: // sh
#if DISALLOW_MISALIGNED
    if (addr & 1) {
#if USE_PRINT
      fprintf(stderr, "misaligned load at pc %X (addr %0X) (instruction %X)\n",
              pc, addr, instruction);
#endif
      return ERR_MISALIGNED_MEMORY_ACCESS;
    }
#endif
    *(uint16_t *)(wmem + addr) = (uint16_t)v2;
    break;
  case 2: // sw
#if DISALLOW_MISALIGNED
    if (addr & 3) {
#if USE_PRINT
      fprintf(stderr, "misaligned load at pc %X (addr %0X) (instruction %X)\n",
              pc, addr, instruction);
#endif
      return ERR_MISALIGNED_MEMORY_ACCESS;
    }
#endif
    *(uint32_t *)(wmem + addr) = v2;
    break;
  default:
    break;
  }
  return 0;
}

static inline void op_int_op(uint32_t *registers, uint32_t instruction) {
  const uint8_t rd = GET_RD(instruction);
  if (rd == 0) {
    return;
  }
  const uint8_t funct3 = GET_FUNCT3(instruction);
  const uint8_t funct7 = GET_FUNCT7(instruction);
  const uint32_t v1 = registers[GET_RS1(instruction)];
  const uint32_t v2 = registers[GET_RS2(instruction)];
  if (funct7 == 1) { // M extension
    switch (funct3) {
    case 0: // mul
      registers[rd] = (int32_t)v1 * (int32_t)v2;
      break;
    case 1: // mulh
      registers[rd] = ((int64_t)(int32_t)v1 * (int64_t)(int32_t)v2) >> 32;
      break;
    case 2: // mulhsu
      registers[rd] = ((uint64_t)((int64_t)(int32_t)v1 * (uint64_t)v2)) >> 32;
      break;
    case 3: // mulhu
      registers[rd] = ((uint64_t)v1 * (uint64_t)v2) >> 32;
      break;
    case 4: // div
    {
      if (v2 == 0) {
        registers[rd] = 0xFFFFFFFF;
      } else if (v1 == INT_MIN_HEX && (int32_t)v2 == -1) {
        // integer overflow
        registers[rd] = v1;
      } else {
        registers[rd] = (int32_t)v1 / (int32_t)v2;
      }
      break;
    }
    case 5: // divu
      if (v2 == 0) {
        registers[rd] = 0xFFFFFFFF;
      } else {
        registers[rd] = v1 / v2;
      }
      break;
    case 6: // rem
    {
      if (v2 == 0) {
        registers[rd] = v1;
      } else if (v1 == INT_MIN_HEX && (int32_t)v2 == -1) {
        // integer overflow
        registers[rd] = 0;
      } else {
        registers[rd] = (int32_t)v1 % (int32_t)v2;
      }
      break;
    }
    case 7: // remu
    {
      if (v2 == 0) {
        registers[rd] = v1;
        // } else if (v1 == INT_MIN_HEX && ()v2 == -1) {
        //   // integer overflow
        //   registers[rd] = 0;
      } else {
        registers[rd] = v1 % v2;
      }
      break;
    }
    default:
      break;
    }
  } else {
    switch (funct3) {
    case 0: // add/sub
      if (funct7 == 0) {
        registers[rd] = v1 + v2;
      } else {
        registers[rd] = v1 - v2;
      }
      break;
    case 1: // sll
      registers[rd] = v1 << (v2 & 0x1F);
      break;
    case 2: // slt
      registers[rd] = (int32_t)v1 < (int32_t)v2 ? 1 : 0;
      break;
    case 3: // sltu
      registers[rd] = v1 < v2 ? 1 : 0;
      break;
    case 4: // xor
      registers[rd] = v1 ^ v2;
      break;
    case 5: // srl and sra
      if (funct7 == 0) {
        // srl
        registers[rd] = v1 >> (v2 & 0x1F);
      } else if (funct7 == 32) {
        // sra
        registers[rd] = ((int32_t)v1) >> (v2 & 0x1F);
      }
      break;
    case 6: // or
      registers[rd] = v1 | v2;
      break;
    case 7: // and
      registers[rd] = v1 & v2;
      break;
    default:
      break;
    }
  }
}

static inline void op_lui(uint32_t *registers, uint32_t instruction) {
  const uint8_t rd = GET_RD(instruction);
  if (rd) {
    registers[rd] = GET_IMM_U(instruction);
  }
}

static inline uint8_t op_branch(uint32_t *registers, uint32_t instruction,
                                uint32_t *pc, int *exit_code) {
  const uint32_t v1 = registers[GET_RS1(instruction)];
  const uint32_t v2 = registers[GET_RS2(instruction)];

  const uint32_t imm =
      (instruction & 0x80000000) | ((instruction & (1 << 7)) << 23) |
      ((instruction & 0x7E000000) >> 1) | ((instruction & 0xF00) << 12);
  const int32_t imms = ((int32_t)imm) >> 19;

  uint8_t is_taken = 0;
  switch (GET_FUNCT3(instruction)) {
  case 0: // beq
    is_taken = v1 == v2;
    break;
  case 1: // bne
    is_taken = v1 != v2;
    break;
  case 4: // blt
    is_taken = (int32_t)v1 < (int32_t)v2;
    break;
  case 5: // bge
    is_taken = (int32_t)v1 >= (int32_t)v2;
    break;
  case 6: // bltu
    is_taken = v1 < v2;
    break;
  case 7: // bgeu
    is_taken = v1 >= v2;
    break;
  }
#if LOG_TRACE
  printf("PC 0x%0X is_taken %d imms %d instruction 0x%04X\n", *pc, is_taken,
         imms, instruction);
#endif
  if (is_taken) {
    if (imms & 3) {
#if USE_PRINT
      fprintf(
          stderr,
          "misaligned branch at pc %0X (instruction %X) address change %d\n",
          *pc, instruction, imms);
#endif
      *exit_code = ERR_MISALIGNED_MEMORY_ACCESS;
      return 0;
    }
    *pc += imms;
  }
  return is_taken;
}

static inline void op_jalr(uint32_t *registers, uint32_t instruction,
                           uint32_t *pc, int *exit_code) {
  const uint8_t rd = GET_RD(instruction);

  const uint32_t addr =
      (registers[GET_RS1(instruction)] + GET_IMM_I(instruction)) & 0xFFFFFFFE;

  if (rd) {
    registers[rd] = *pc + 4;
  }
  if (addr & 0x3) {
#if USE_PRINT
    fprintf(stderr,
            "misaligned jalr at pc %0X (instruction %X) new address 0x%0X\n",
            *pc, instruction, addr);
#endif
    *exit_code = ERR_MISALIGNED_MEMORY_ACCESS;
  }
  *pc = addr;
}

static inline void op_jal(uint32_t *registers, uint32_t instruction,
                          uint32_t *pc, int *exit_code) {

  const uint8_t rd = GET_RD(instruction);
  const uint32_t imm =
      ((instruction << 11) & 0x7F800000) | ((instruction << 2) & (1 << 22)) |
      ((instruction >> 9) & 0x3FF000) | (instruction & (1 << 31));
  int32_t immi = ((int32_t)imm) >> 11;

  if (rd) {
    registers[rd] = *pc + 4;
  }
  if (immi & 0x3) {
#if USE_PRINT
    fprintf(stderr,
            "misaligned jal at pc %0X (instruction %X) address change %d\n",
            *pc, instruction, immi);
#endif
    *exit_code = ERR_MISALIGNED_MEMORY_ACCESS;
  }
  *pc += immi;
}

static inline int op_ecall(uint32_t *registers, uint32_t instruction,
                           int *ext_exit_code) {
  uint32_t gp = registers[3];
  uint32_t a0 = registers[10]; // argument
  uint32_t a7 = registers[17]; // function
  uint8_t is_test;
  uint32_t exit_code;
  switch (a7) {
  case 93:
    // exit
    is_test = a0 & 1;
    exit_code = a0 >> 1;
    if (is_test && a0) {
#if USE_PRINT
      fprintf(stderr, "*** FAILED *** (tohost = %d)\n", exit_code);
#endif
    }
#if USE_PRINT
    printf("exit code %d\n", exit_code);
#endif
    *ext_exit_code = exit_code;
    return 1;
    break;
  case 63:
    // read
    // printf("read\n");
    // reg[a0] = getchar();
    break;
  case 64:
    // write
//  printf("write %c\n", a0);
#if USE_PRINT
    putchar(a0);
#endif
    break;
  default:
    break;
  }
  return 0;
}

static inline int op_system(uint32_t *registers, uint32_t instruction,
                            int *ext_exit_code) {
  const uint8_t funct3 = GET_FUNCT3(instruction);
  const uint8_t funct7 = GET_FUNCT7(instruction);
  const uint32_t v1 = registers[GET_RS1(instruction)];
  const uint32_t v2 = registers[GET_RS2(instruction)];

  if (funct3 == 0x0) {
    switch (funct7) {
    case 0x0: // ecall
      return op_ecall(registers, instruction, ext_exit_code);
      break;
    case 0x1: // ebreak
#if USE_PRINT
      fprintf(stderr, "ebreak, exiting\n");
#endif
      *ext_exit_code = ERR_EBREAK;
      return 1;
      break;
    case 0x8: // wfi
      *ext_exit_code = ERR_WFI;
#if USE_PRINT
      fprintf(stderr, "wfi instruction, exiting\n");
#endif
      return 1;
      break;
    default:
      break;
    }
    return 0;
  }
  const uint8_t rd = GET_RD(instruction);
  const uint8_t rs1 = GET_RS1(instruction);
  const uint32_t csr = (instruction >> 20) & 0xFFF;
#if USE_PRINT && LOG_TRACE
  printf("rd %02d rs1 %02d csr %03X\n", rd, rs1, csr);
#endif

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
      if (rd) {
        registers[rd] = 0;
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
  return 0;
}

static void dump_registers(uint8_t *registers) {
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

void dbg_dump_registers_short(uint8_t *wmem) {
  uint32_t *reg = (uint32_t *)wmem;
  for (int i = 0; i < 32; i++) {
    uint32_t val = reg[i];
    if (val) {
      printf("%02d 0x%04X ", i, val);
    }
  }
  printf("\n");
}