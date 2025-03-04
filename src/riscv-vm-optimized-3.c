#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cycle-counter.h"
#include "riscv-vm-common.h"
#include "riscv-vm-optimized-1.h"

#define USE_ZMM_REGISTERS 0

#if USE_ZMM_REGISTERS
#include <immintrin.h>

/*
uint32_t extract_lower32(__m512i zmm) {
    __m128i xmm = _mm512_castsi512_si128(zmm);    // Downcast ZMM to XMM
    return _mm_cvtsi128_si32(xmm);                // Extract lower 32 bits as
uint32_t
}

uint32_t extract_lower32_alternative(__m512i zmm) {
    return _mm512_maskz_extracti32x4_epi32(1, zmm, 0)[0];
}
*/

uint32_t extract_last_32_bits(__m512i zmm) {
  __m128i xmm =
      _mm512_extracti32x4_epi32(zmm, 3); // Extract lane 3 (last 128 bits)
  _mm_srli_si128(xmm, 12);
  return _mm_cvtsi128_si32(xmm); // Get the lowest 32 bits of xmm
}

inline uint32_t zmm_extract_32_bits(__m512i zmm, int index) {
  // __m128i xmm = _mm512_extracti32x4_epi32(zmm, index >> 2); // Extract lane 3
  // (last 128 bits)
  __m128i xmm;
  switch (index >> 2) {
  case 0:
    xmm = _mm512_extracti32x4_epi32(zmm, 0);
    break;
  case 1:
    xmm = _mm512_extracti32x4_epi32(zmm, 1);
    break;
  case 2:
    xmm = _mm512_extracti32x4_epi32(zmm, 2);
    break;
  case 3:
    xmm = _mm512_extracti32x4_epi32(zmm, 3);
    break;
  }
  //__m128i xmm = _mm512_extracti32x4_epi32(zmm, index >> 2); // Extract lane 3
  //(last 128 bits)

  _mm_srli_si128(xmm, (index & 3) * 4);
  return _mm_cvtsi128_si32(xmm); // Get the lowest 32 bits of xmm
}

inline __m512i zmm_set_32_bit(__m512i zmm, int index, uint32_t val) {
  // __m512i scalar = _mm_set1_epi32(val); // Load scalar value
  // __mmask16 mask = 0b1000000000000000;               // Mask: update only the
  // high 32-bit element
  __mmask16 mask = 1 << index;
  // return _mm512_mask_broadcastd_epi32(zmm, mask, scalar);
  return _mm512_mask_set1_epi32(zmm, mask, val);
}

// __m512i _mm512_insert_epi16(__m512i target, const std::int16_t x, const int
// index)
// {
//     return _mm512_mask_set1_epi16(target, 1UL << index, x);
// }
// static inline __m512i _mm512_insert_epi32(__m512i target, const std::int32_t
// x, const int index)
// {
//     return _mm512_mask_set1_epi32(target, 1UL << index, x);
// }

// template <int index>
// int _mm512_extract_epi32(__m512i target)
// {
//     return _mm512_cvtsi512_si32(_mm512_alignr_epi32(target, target, index));
// }

// template <int index>
// int  _mm512_extract_epi16(__m512i target)
// {
//     return (_mm512_extract_epi32<index / 2>(target) >> (index % 2 ? 16 : 0))
//     & 0xFFFF;
// }

#define GET_FROM_REG(dest, regnum)                                             \
  {                                                                            \
    if (regnum & 1) {                                                          \
      dest = zmm_extract_32_bits(zreg1, regnum >> 1);                          \
    } else {                                                                   \
      dest = zmm_extract_32_bits(zreg2, regnum >> 1);                          \
    }                                                                          \
  }

#define SET_TO_REG(regnum, val)                                                \
  {                                                                            \
    if (regnum & 1) {                                                          \
      zreg1 = zmm_set_32_bit(zreg1, regnum >> 1, val);                         \
    } else {                                                                   \
      zreg1 = zmm_set_32_bit(zreg2, regnum >> 1, val);                         \
    }                                                                          \
  }

#endif

// 1 MiB, memory available inside VM
// static const int VM_MEMORY = 1048576;
static const int VM_MEMORY = 1048576 * 16;

static const size_t work_mem_size = VM_MEMORY * 1; // size of memory to allocate

#if LOG_TRACE
static void dbg_dump_registers_short(uint32_t *reg);
#endif

static int riscv_vm_main_loop_3(uint8_t *initial_registers, uint8_t *wmem,
                                uint32_t *pcp_out);

static uint64_t mcycle_val = 0;
static uint64_t start_time = 0;
static uint64_t duration = 0;
static double speed = 0;

int riscv_vm_run_optimized_3(uint8_t *registers, uint8_t *program,
                             uint32_t program_len) {

#if USE_PRINT
  printf("Starting VM... work mem size %zu program len %d\n", work_mem_size,
         program_len);
#endif
  if (program_len > work_mem_size) {
#if USE_PRINT
    fprintf(stderr,
            "Program too long for memory, program size %d memory size %zu\n",
            program_len, work_mem_size);
#endif
    return ERR_OUT_OF_MEM;
  }
  init_counter();
  start_time = get_cycles();
  printf("start time %" PRIu64 "\n", start_time);
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
#if PRINT_REGISTERS
  dump_registers(registers);
#endif
  int res = riscv_vm_main_loop_3(registers, wmem, &pcp);
#if PRINT_REGISTERS
  dump_registers(registers);
#endif
#if USE_PRINT
  printf("Final PC: %04X\n", pcp);
#endif
  free(wmem);
  return res;
}

#if USE_ZMM_REGISTERS
#define XGET_FROM_REG(dest, regnum)                                            \
  ({                                                                           \
    switch (regnum) {                                                          \
    case 0:                                                                    \
      asm volatile("vmovd %%xmm0, %0" : "=r"(dest) : : "%xmm0");               \
      break;                                                                   \
    case 1:                                                                    \
      asm volatile("vmovd %%xmm1, %0" : "=r"(dest) : : "%xmm1");               \
      break;                                                                   \
    case 2:                                                                    \
      asm volatile("vmovd %%xmm2, %0" : "=r"(dest) : : "%xmm2");               \
      break;                                                                   \
    case 3:                                                                    \
      asm volatile("vmovd %%xmm3, %0" : "=r"(dest) : : "%xmm3");               \
      break;                                                                   \
    case 4:                                                                    \
      asm volatile("vmovd %%xmm4, %0" : "=r"(dest) : : "%xmm4");               \
      break;                                                                   \
    case 5:                                                                    \
      asm volatile("vmovd %%xmm5, %0" : "=r"(dest) : : "%xmm5");               \
      break;                                                                   \
    case 6:                                                                    \
      asm volatile("vmovd %%xmm6, %0" : "=r"(dest) : :);                       \
      break;                                                                   \
    case 7:                                                                    \
      asm volatile("vmovd %%xmm7, %0" : "=r"(dest) : :);                       \
      break;                                                                   \
    case 8:                                                                    \
      asm volatile("vmovd %%xmm8, %0" : "=r"(dest) : :);                       \
      break;                                                                   \
    case 9:                                                                    \
      asm volatile("vmovd %%xmm9, %0" : "=r"(dest) : :);                       \
      break;                                                                   \
    case 10:                                                                   \
      asm volatile("vmovd %%xmm10, %0" : "=r"(dest) : :);                      \
      break;                                                                   \
    case 11:                                                                   \
      asm volatile("vmovd %%xmm11, %0" : "=r"(dest) : :);                      \
      break;                                                                   \
    case 12:                                                                   \
      asm volatile("vmovd %%xmm12, %0" : "=r"(dest) : :);                      \
      break;                                                                   \
    case 13:                                                                   \
      asm volatile("vmovd %%xmm13, %0" : "=r"(dest) : :);                      \
      break;                                                                   \
    case 14:                                                                   \
      asm volatile("vmovd %%xmm14, %0" : "=r"(dest) : :);                      \
      break;                                                                   \
    case 15:                                                                   \
      asm volatile("vmovd %%xmm15, %0" : "=r"(dest) : :);                      \
      break;                                                                   \
    case 16:                                                                   \
      asm volatile("vmovd %%xmm16, %0" : "=r"(dest) : :);                      \
      break;                                                                   \
    case 17:                                                                   \
      asm volatile("vmovd %%xmm17, %0" : "=r"(dest) : :);                      \
      break;                                                                   \
    case 18:                                                                   \
      asm volatile("vmovd %%xmm18, %0" : "=r"(dest) : :);                      \
      break;                                                                   \
    case 19:                                                                   \
      asm volatile("vmovd %%xmm19, %0" : "=r"(dest) : :);                      \
      break;                                                                   \
    case 20:                                                                   \
      asm volatile("vmovd %%xmm20, %0" : "=r"(dest) : :);                      \
      break;                                                                   \
    case 21:                                                                   \
      asm volatile("vmovd %%xmm21, %0" : "=r"(dest) : :);                      \
      break;                                                                   \
    case 22:                                                                   \
      asm volatile("vmovd %%xmm22, %0" : "=r"(dest) : :);                      \
      break;                                                                   \
    case 23:                                                                   \
      asm volatile("vmovd %%xmm23, %0" : "=r"(dest) : :);                      \
      break;                                                                   \
    case 24:                                                                   \
      asm volatile("vmovd %%xmm24, %0" : "=r"(dest) : :);                      \
      break;                                                                   \
    case 25:                                                                   \
      asm volatile("vmovd %%xmm25, %0" : "=r"(dest) : :);                      \
      break;                                                                   \
    case 26:                                                                   \
      asm volatile("vmovd %%xmm26, %0" : "=r"(dest) : :);                      \
      break;                                                                   \
    case 27:                                                                   \
      asm volatile("vmovd %%xmm27, %0" : "=r"(dest) : :);                      \
      break;                                                                   \
    case 28:                                                                   \
      asm volatile("vmovd %%xmm28, %0" : "=r"(dest) : :);                      \
      break;                                                                   \
    case 29:                                                                   \
      asm volatile("vmovd %%xmm29, %0" : "=r"(dest) : :);                      \
      break;                                                                   \
    case 30:                                                                   \
      asm volatile("vmovd %%xmm30, %0" : "=r"(dest) : :);                      \
      break;                                                                   \
    case 31:                                                                   \
      asm volatile("vmovd %%xmm31, %0" : "=r"(dest) : :);                      \
      break;                                                                   \
    }                                                                          \
  })

#define XSET_TO_REG(regnum, val)                                               \
  {                                                                            \
    {                                                                          \
      uint32_t __temp = val;                                                   \
      switch (regnum) {                                                        \
      case 0:                                                                  \
        asm volatile("vmovd %0, %%zmm0" : : "r"(__temp));                      \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
  }
#define ZSET_TO_REG(regnum, val)                                               \
  {                                                                            \
    {                                                                          \
      uint32_t __temp = val;                                                   \
      switch (regnum) {                                                        \
      case 0:                                                                  \
        asm volatile("vmovd %0, %%xmm0" : : "r"(__temp) : "%xmm0");            \
        break;                                                                 \
      case 1:                                                                  \
        asm volatile("vmovd %0, %%xmm1" : : "r"(__temp) : "%xmm1");            \
        break;                                                                 \
      case 2:                                                                  \
        asm volatile("vmovd %0, %%xmm2" : : "r"(__temp));                      \
        break;                                                                 \
      case 3:                                                                  \
        asm volatile("vmovd %0, %%xmm3" : : "r"(__temp));                      \
        break;                                                                 \
      case 4:                                                                  \
        asm volatile("vmovd %0, %%xmm4" : : "r"(__temp));                      \
        break;                                                                 \
      case 5:                                                                  \
        asm volatile("vmovd %0, %%xmm5" : : "r"(__temp));                      \
        break;                                                                 \
      case 6:                                                                  \
        asm volatile("vmovd %0, %%xmm6" : : "r"(__temp));                      \
        break;                                                                 \
      case 7:                                                                  \
        asm volatile("vmovd %0, %%xmm7" : : "r"(__temp));                      \
        break;                                                                 \
      case 8:                                                                  \
        asm volatile("vmovd %0, %%xmm8" : : "r"(__temp));                      \
        break;                                                                 \
      case 9:                                                                  \
        asm volatile("vmovd %0, %%xmm9" : : "r"(__temp));                      \
        break;                                                                 \
      case 10:                                                                 \
        asm volatile("vmovd %0, %%xmm10" : : "r"(__temp));                     \
        break;                                                                 \
      case 11:                                                                 \
        asm volatile("vmovd %0, %%xmm11" : : "r"(__temp));                     \
        break;                                                                 \
      case 12:                                                                 \
        asm volatile("vmovd %0, %%xmm12" : : "r"(__temp));                     \
        break;                                                                 \
      case 13:                                                                 \
        asm volatile("vmovd %0, %%xmm13" : : "r"(__temp));                     \
        break;                                                                 \
      case 14:                                                                 \
        asm volatile("vmovd %0, %%xmm14" : : "r"(__temp));                     \
        break;                                                                 \
      case 15:                                                                 \
        asm volatile("vmovd %0, %%xmm15" : : "r"(__temp));                     \
        break;                                                                 \
      case 16:                                                                 \
        asm volatile("vmovd %0, %%xmm16" : : "r"(__temp));                     \
        break;                                                                 \
      case 17:                                                                 \
        asm volatile("vmovd %0, %%xmm17" : : "r"(__temp));                     \
        break;                                                                 \
      case 18:                                                                 \
        asm volatile("vmovd %0, %%xmm18" : : "r"(__temp));                     \
        break;                                                                 \
      case 19:                                                                 \
        asm volatile("vmovd %0, %%xmm19" : : "r"(__temp));                     \
        break;                                                                 \
      case 20:                                                                 \
        asm volatile("vmovd %0, %%xmm20" : : "r"(__temp));                     \
        break;                                                                 \
      case 21:                                                                 \
        asm volatile("vmovd %0, %%xmm21" : : "r"(__temp));                     \
        break;                                                                 \
      case 22:                                                                 \
        asm volatile("vmovd %0, %%xmm22" : : "r"(__temp));                     \
        break;                                                                 \
      case 23:                                                                 \
        asm volatile("vmovd %0, %%xmm23" : : "r"(__temp));                     \
        break;                                                                 \
      case 24:                                                                 \
        asm volatile("vmovd %0, %%xmm24" : : "r"(__temp));                     \
        break;                                                                 \
      case 25:                                                                 \
        asm volatile("vmovd %0, %%xmm25" : : "r"(__temp));                     \
        break;                                                                 \
      case 26:                                                                 \
        asm volatile("vmovd %0, %%xmm26" : : "r"(__temp));                     \
        break;                                                                 \
      case 27:                                                                 \
        asm volatile("vmovd %0, %%xmm27" : : "r"(__temp));                     \
        break;                                                                 \
      case 28:                                                                 \
        asm volatile("vmovd %0, %%xmm28" : : "r"(__temp));                     \
        break;                                                                 \
      case 29:                                                                 \
        asm volatile("vmovd %0, %%xmm29" : : "r"(__temp));                     \
        break;                                                                 \
      case 30:                                                                 \
        asm volatile("vmovd %0, %%xmm30" : : "r"(__temp));                     \
        break;                                                                 \
      case 31:                                                                 \
        asm volatile("vmovd %0, %%xmm31" : : "r"(__temp));                     \
        break;                                                                 \
      default:                                                                 \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
  }

#else
#define GET_FROM_REG(dest, regnum) (dest = registers[regnum])
#define SET_TO_REG(regnum, val) (registers[regnum] = val)

#endif

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

#define exit_loop(ec)                                                          \
  memcpy(initial_registers, registers, REG_MEM_SIZE);                          \
  *pcp_out = pc;                                                               \
  duration = get_cycles() - start_time;                                        \
  speed = (double)mcycle_val / ((double)duration / 1e9);                       \
  printf("system exit mcycle=%" PRIu64 " dur %" PRIu64                         \
         " speed is %g ops/sec (%f "                                           \
         "nanosec/inst)\n",                                                    \
         mcycle_val, duration, speed,                                          \
         ((double)duration / 1.0) / (double)mcycle_val);                       \
  return ec;

static int riscv_vm_main_loop_3(uint8_t *initial_registers, uint8_t *wmem,
                                uint32_t *pcp_out) {
  uint32_t registers[32];
  uint8_t *program = wmem;
  uint32_t pc = 0;
  uint32_t instruction;
  int res = 0;
  memcpy(registers, initial_registers, REG_MEM_SIZE);

#if USE_ZMM_REGISTERS
  __m512i zreg1 = _mm512_setzero_si512();
  __m512i zreg2 = _mm512_setzero_si512();
  // __m512i zregt;
  {
    printf("=====> registers:");
    for (int i = 0; i < 32; i++) {
      uint32_t val;
      GET_FROM_REG(val, i);
      if (val) {
        printf("%02d 0x%04X ", i, val);
      }
    }
    printf("\n");
  }
  printf("=====> done\n");

  {
    uint32_t zero = 10;
    for (int i = 0; i < 32; i++) {
      SET_TO_REG(i, zero);
    }
  }
  {
    printf("=====> registers:");
    for (int i = 0; i < 32; i++) {
      uint32_t val;
      GET_FROM_REG(val, i);
      if (val) {
        printf("%02d 0x%04X ", i, val);
      }
    }
    printf("\n");
  }
  printf("=====> done\n");
#endif
  static void *op_table[] = {
      &&uo,         &&uo,        &&uo, &&op_load,  &&uo,
      &&uo,         &&uo,        &&uo, &&uo,       &&uo,
      &&uo,         &&uo,        &&uo, &&uo,       &&uo,
      &&normal_end, &&uo,        &&uo, &&uo,       &&op_int_imm_op,
      &&uo,         &&uo,        &&uo, &&op_auipc, &&uo,
      &&uo,         &&uo,        &&uo, &&uo,       &&uo,
      &&uo,         &&uo,        &&uo, &&uo,       &&uo,
      &&op_store,   &&uo,        &&uo, &&uo,       &&uo,
      &&uo,         &&uo,        &&uo, &&uo,       &&uo,
      &&uo,         &&uo,        &&uo, &&uo,       &&uo,
      &&uo,         &&op_int_op, &&uo, &&uo,       &&uo,
      &&op_lui,     &&uo,        &&uo, &&uo,       &&uo,
      &&uo,         &&uo,        &&uo, &&uo,       &&uo,
      &&uo,         &&uo,        &&uo, &&uo,       &&uo,
      &&uo,         &&uo,        &&uo, &&uo,       &&uo,
      &&uo,         &&uo,        &&uo, &&uo,       &&uo,
      &&uo,         &&uo,        &&uo, &&uo,       &&uo,
      &&uo,         &&uo,        &&uo, &&uo,       &&uo,
      &&uo,         &&uo,        &&uo, &&uo,       &&uo,
      &&uo,         &&uo,        &&uo, &&uo,       &&op_branch,
      &&uo,         &&uo,        &&uo, &&op_jalr,  &&uo,
      &&uo,         &&uo,        &&uo, &&uo,       &&uo,
      &&uo,         &&op_jal,    &&uo, &&uo,       &&uo,
      &&op_system,  &&uo,        &&uo, &&uo,       &&uo,
      &&uo,         &&uo,        &&uo, &&uo,       &&uo,
      &&uo,         &&uo,        &&uo, &&uo,       &&uo,
      &&uo,         &&uo,        &&uo, &&uo,       &&uo,
      &&uo};

  while (res == 0) {
    mcycle_val++;
    instruction = *(uint32_t *)(program + pc);
#if LOG_TRACE
    printf("PC: 0x%04X inst 0x%08X ", pc, instruction);
    dbg_dump_registers_short(registers);
    //  printf("\n");
#endif
    {
      for (int i = 0; i < 32; i++) {
        uint32_t val;
        GET_FROM_REG(val, i);
        if (val) {
          printf("%02d 0x%04X ", i, val);
        }
      }
      printf("\n");
    }

    goto *op_table[instruction & 0x7F];
  /*
    switch (instruction & 0x7F) {
    case 0x03:
      // if ((res = op_load(registers, program, instruction, pc))) {
      //   exit_loop(res);
      // }
      goto op_load;
      break;
    case 0x0F:
      // misc-mem opcode, nop
      break;
    case 0x13:
      goto op_int_imm_op;
      break;
    case 0x17:
      goto op_auipc;
      break;
    case 0x23:
      goto op_store;
      break;
    case 0x33:
      goto op_int_op;
      break;
    case 0x37:
      goto op_lui;
      break;
    case 0x63:
      goto op_branch;
      break;
    case 0x67:
      goto op_jalr;
      continue;
    case 0x6f:
      goto op_jal;
      continue;
    case 0x73:
      goto op_system;
      break;
    default:
#if USE_PRINT
      fprintf(stderr, "Unimplemented or invalid opcode 0x%02X at PC 0x%02X\n",
              instruction & 0x7F, pc);
#endif
      exit_loop(ERR_UNIMPLEMENTED_OPCODE) break;
    }
    */
  normal_end:;
    pc += 4;
  jump_end:;
  }
  exit_loop(res);
  {
  uo:;
#if USE_PRINT
    fprintf(stderr, "Unimplemented or invalid opcode 0x%02X at PC 0x%02X\n",
            instruction & 0x7F, pc);
#endif
    exit_loop(ERR_UNIMPLEMENTED_OPCODE);
  }

  {
  op_load:;
    const uint8_t rd = GET_RD(instruction);
    if (rd == 0) {
      goto normal_end;
    }
    /*
    // uint32_t addr = GET_FROM_REG(GET_RS1(instruction)) +
    // GET_IMM_I(instruction);
    */
    uint32_t addr;
    GET_FROM_REG(addr, GET_RS1(instruction));
    addr += GET_IMM_I(instruction);
#if LOG_TRACE
    printf("op_load addr 0x%x imm %d\n", addr, GET_IMM_I(instruction));
#endif
    // hack to account for code that uses absoulte addresses and start
    // virtual memory from 0x80000000
    addr &= 0x7FFFFFFF;
    if (__builtin_expect(addr >= work_mem_size, 0)) {
#if USE_PRINT
      fprintf(stderr,
              "load went beyond allocated memory pc %X (addr %0X) (instruction "
              "%X)\n",
              pc, addr, instruction);
#endif
      exit_loop(ERR_INVALID_MEMORY_ACCESS);
    }
    switch (GET_FUNCT3(instruction)) {
    case 0: // lb
      SET_TO_REG(rd, (int8_t)wmem[addr]);
      break;
    case 1: // lh
#if DISALLOW_MISALIGNED
      if (addr & 1) {
#if USE_PRINT
        fprintf(stderr,
                "misaligned load at pc %X (addr %0X) (instruction %X)\n", pc,
                addr, instruction);
#endif
        exit_loop(ERR_MISALIGNED_MEMORY_ACCESS);
      }
#endif
      SET_TO_REG(rd, *(int16_t *)(wmem + addr));
      break;
    case 2: // lw
#if DISALLOW_MISALIGNED
      if (addr & 3) {
#if USE_PRINT
        fprintf(stderr,
                "misaligned load at pc 0x%X (addr 0x%0X) (instruction 0x%X)\n",
                pc, addr, instruction);
#endif
        exit_loop(ERR_MISALIGNED_MEMORY_ACCESS);
      }
#endif
      SET_TO_REG(rd, *(uint32_t *)(wmem + addr));
      break;
    case 4: // lbu
      SET_TO_REG(rd, wmem[addr]);
      break;
    case 5: // lhu
#if DISALLOW_MISALIGNED
      if (addr & 1) {
#if USE_PRINT
        fprintf(stderr,
                "misaligned load at pc 0x%X (addr 0x%0X) (instruction 0x%X)\n",
                pc, addr, instruction);
#endif
        exit_loop(ERR_MISALIGNED_MEMORY_ACCESS);
      }
#endif
      SET_TO_REG(rd, *(uint16_t *)(wmem + addr));
      break;
    default:
      break;
    }
    goto normal_end;
  }
  {
  op_int_imm_op:;
    const uint8_t rd = GET_RD(instruction);
    if (rd == 0) {
      goto normal_end;
    }
    uint8_t shift;
    const uint8_t rs1 = GET_RS1(instruction);
    int32_t immi = GET_IMM_I(instruction);
    /*
    // uint32_t v1 = GET_FROM_REG(rs1);
    */
    uint32_t v1;
    GET_FROM_REG(v1, rs1);
#if LOG_TRACE
    printf("op imm funct3=%d rd=%d rs1=%d rs2=%d imms=%d\n",
           GET_FUNCT3(instruction), rd, GET_RS1(instruction),
           GET_RS2(instruction), immi);
#endif
    switch (GET_FUNCT3(instruction)) {
    case 0: // addi
      SET_TO_REG(rd, v1 + immi);
      break;
    case 2: // slti
      SET_TO_REG(rd, (int32_t)v1 < immi ? 1 : 0);
      break;
    case 3: // sltiu
      SET_TO_REG(rd, v1 < ((uint32_t)immi) ? 1 : 0);
      break;
    case 4: // xori
      SET_TO_REG(rd, v1 ^ immi);
      break;
    case 6: // ori
      SET_TO_REG(rd, v1 | immi);
      break;
    case 7: // andi
      SET_TO_REG(rd, v1 & immi);
      break;
    case 1: // slli
      shift = (instruction >> 20) & 0x1f;
      SET_TO_REG(rd, v1 << shift);
      break;
    case 5: // srli and srai
      shift = (instruction >> 20) & 0x1f;
      if ((instruction >> 30) & 1) {
        // srai
        registers[rd] = (int32_t)v1 >> shift;
      } else {
        // srli
        registers[rd] = v1 >> shift;
      }
      break;
    default:
      break;
    }
    goto normal_end;
  }
  {
  op_auipc:;
    const uint8_t rd = GET_RD(instruction);
    if (rd != 0) {
      SET_TO_REG(rd, pc + GET_IMM_U(instruction));
    }
    goto normal_end;
  }
  {
  op_store:;
    const uint8_t funct3 = GET_FUNCT3(instruction);
    /*
    // const uint32_t v2 = GET_FROM_REG(GET_RS2(instruction));
    // uint32_t addr = GET_FROM_REG(GET_RS1(instruction)) +
    // GET_IMM_S(instruction);
    */
    uint32_t v2;
    GET_FROM_REG(v2, GET_RS2(instruction));
    uint32_t addr;
    GET_FROM_REG(addr, GET_RS1(instruction));
    addr += GET_IMM_S(instruction);
#if LOG_TRACE
    printf("op_store addr 0x%x imm %d\n", addr, GET_IMM_S(instruction));
#endif

#if EMULATE_UART_OUT
    if (addr == UART_OUT_REGISTER) {
      uint8_t byte;
      // uint16_t half;
      // uint32_t word;
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
      goto normal_end;
    }
#endif
    // hack to account for code that uses absoulte addresses and start
    // virtual memory from 0x80000000
    addr &= 0x7FFFFFFF;

    if (__builtin_expect(addr >= work_mem_size, 0)) {
#if USE_PRINT
      fprintf(
          stderr,
          "store went beyond allocated memory pc %X (addr %0X) (instruction "
          "%X)\n",
          pc, addr, instruction);
#endif
      exit_loop(ERR_INVALID_MEMORY_ACCESS);
    }
#if USE_TOHOST_SYSCALL
    // used by benchmarks in https://github.com/riscv-software-src/riscv-tests
    if (addr == tohost) {
      uint32_t from_virt = v2;
      uint8_t is_exit = from_virt & 1;
      if (is_exit) {
        exit_loop(from_virt >> 1);
      }
      from_virt = *(uint32_t *)(wmem + v2);
      if (from_virt == SYS_write) {
        // uint32_t a0 = *(uint32_t *)(wmem + v2 + 8);
        uint32_t str_ptr = *(uint32_t *)(wmem + v2 + 16);
        uint32_t str_len = *(uint32_t *)(wmem + v2 + 24);
        for (uint32_t i = 0; i < str_len; i++) {
          printf("%c", wmem[str_ptr + i]);
        }
      } else {
#if USE_PRINT
        fprintf(stderr, "unimplemented magic syscall %d\n", from_virt);
#endif
        exit_loop(ERR_UNIMPLEMENTED_MAGIC_SYSCALL);
      }
      wmem[fromhost] = 1;
      goto normal_end;
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
        fprintf(stderr,
                "misaligned load at pc %X (addr %0X) (instruction %X)\n", pc,
                addr, instruction);
#endif
        exit_loop(ERR_MISALIGNED_MEMORY_ACCESS);
      }
#endif
      *(uint16_t *)(wmem + addr) = (uint16_t)v2;
      break;
    case 2: // sw
#if DISALLOW_MISALIGNED
      if (addr & 3) {
#if USE_PRINT
        fprintf(stderr,
                "misaligned load at pc %X (addr %0X) (instruction %X)\n", pc,
                addr, instruction);
#endif
        exit_loop(ERR_MISALIGNED_MEMORY_ACCESS);
      }
#endif
      *(uint32_t *)(wmem + addr) = v2;
      break;
    default:
      break;
    }
    goto normal_end;
  }
  {
  op_int_op:;
    const uint8_t rd = GET_RD(instruction);
    if (rd == 0) {
      goto normal_end;
    }
    const uint8_t funct3 = GET_FUNCT3(instruction);
    const uint8_t funct7 = GET_FUNCT7(instruction);
    /*
    // const uint32_t v1 = GET_FROM_REG(GET_RS1(instruction));
    // const uint32_t v2 = GET_FROM_REG(GET_RS2(instruction));
    */
    uint32_t v1;
    GET_FROM_REG(v1, GET_RS1(instruction));
    uint32_t v2;
    GET_FROM_REG(v2, GET_RS2(instruction));
#if LOG_TRACE
    printf("int op funct3=%d funct7=%d rd=%d rs1=%d rs2=%d\n", funct3, funct7,
           rd, GET_RS1(instruction), GET_RS2(instruction));
#endif
    if (funct7 == 1) { // M extension
      switch (funct3) {
      case 0: // mul
        SET_TO_REG(rd, (int32_t)v1 * (int32_t)v2);
        break;
      case 1: // mulh
        SET_TO_REG(rd, ((int64_t)(int32_t)v1 * (int64_t)(int32_t)v2) >> 32);
        break;
      case 2: // mulhsu
        SET_TO_REG(rd, ((uint64_t)((int64_t)(int32_t)v1 * (uint64_t)v2)) >> 32);
        break;
      case 3: // mulhu
        SET_TO_REG(rd, ((uint64_t)v1 * (uint64_t)v2) >> 32);
        break;
      case 4: // div
      {
        if (v2 == 0) {
          SET_TO_REG(rd, 0xFFFFFFFF);
        } else if (v1 == INT_MIN_HEX && (int32_t)v2 == -1) {
          // integer overflow
          SET_TO_REG(rd, v1);
        } else {
          SET_TO_REG(rd, (int32_t)v1 / (int32_t)v2);
        }
        break;
      }
      case 5: // divu
        if (v2 == 0) {
          SET_TO_REG(rd, 0xFFFFFFFF);
        } else {
          SET_TO_REG(rd, v1 / v2);
        }
        break;
      case 6: // rem
      {
        if (v2 == 0) {
          SET_TO_REG(rd, v1);
        } else if (v1 == INT_MIN_HEX && (int32_t)v2 == -1) {
          // integer overflow
          SET_TO_REG(rd, 0);
        } else {
          SET_TO_REG(rd, (int32_t)v1 % (int32_t)v2);
        }
        break;
      }
      case 7: // remu
      {
        if (v2 == 0) {
          SET_TO_REG(rd, v1);
          // } else if (v1 == INT_MIN_HEX && ()v2 == -1) {
          //   // integer overflow
          //   registers[rd] = 0;
        } else {
          SET_TO_REG(rd, v1 % v2);
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
          SET_TO_REG(rd, v1 + v2);
        } else {
          SET_TO_REG(rd, v1 - v2);
        }
        break;
      case 1: // sll
        SET_TO_REG(rd, v1 << (v2 & 0x1F));
        break;
      case 2: // slt
        SET_TO_REG(rd, (int32_t)v1 < (int32_t)v2 ? 1 : 0);
        break;
      case 3: // sltu
        SET_TO_REG(rd, v1 < v2 ? 1 : 0);
        break;
      case 4: // xor
        SET_TO_REG(rd, v1 ^ v2);
        break;
      case 5: // srl and sra
        if (funct7 == 0) {
          // srl
          SET_TO_REG(rd, v1 >> (v2 & 0x1F));
        } else if (funct7 == 32) {
          // sra
          SET_TO_REG(rd, ((int32_t)v1) >> (v2 & 0x1F));
        }
        break;
      case 6: // or
        SET_TO_REG(rd, v1 | v2);
        break;
      case 7: // and
        SET_TO_REG(rd, v1 & v2);
        break;
      default:
        break;
      }
    }
    goto normal_end;
  }
  {
  op_lui:;
    const uint8_t rd = GET_RD(instruction);
#if LOG_TRACE
    printf("lui rd=%d imm=%d\n", rd, GET_IMM_U(instruction));
#endif
    if (rd) {
      SET_TO_REG(rd, GET_IMM_U(instruction));
    }
    goto normal_end;
  }
  {
  op_branch:;
    /*
    // const uint32_t v1 = GET_FROM_REG(GET_RS1(instruction));
    // const uint32_t v2 = GET_FROM_REG(GET_RS2(instruction));
    */
    uint32_t v1;
    GET_FROM_REG(v1, GET_RS1(instruction));
    uint32_t v2;
    GET_FROM_REG(v2, GET_RS2(instruction));

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
    printf("branch rs1=%d rs2=%d imms=%d is_taken=%d\n", GET_RS1(instruction),
           GET_RS2(instruction), imms, is_taken);
    printf("PC 0x%0X is_taken %d imms %d instruction 0x%04X\n", pc, is_taken,
           imms, instruction);
#endif
    if (is_taken) {
      if (imms & 3) {
#if USE_PRINT
        fprintf(
            stderr,
            "misaligned branch at pc %0X (instruction %X) address change %d\n",
            pc, instruction, imms);
#endif
        exit_loop(ERR_MISALIGNED_MEMORY_ACCESS);
      }
      pc += imms;
      goto jump_end;
    }
    goto normal_end;
  }
  {
  op_jalr:;
    const uint8_t rd = GET_RD(instruction);

    /*
    // const uint32_t addr =
    //     (GET_FROM_REG(GET_RS1(instruction)) + GET_IMM_I(instruction)) &
    //     0xFFFFFFFE;
    */
    uint32_t addr;
    GET_FROM_REG(addr, GET_RS1(instruction));
    addr += GET_IMM_I(instruction);
    addr &= 0xFFFFFFFE;

    if (rd) {
      SET_TO_REG(rd, pc + 4);
    }
    if (addr & 0x3) {
#if USE_PRINT
      fprintf(stderr,
              "misaligned jalr at pc %0X (instruction %X) new address 0x%0X\n",
              pc, instruction, addr);
#endif
      exit_loop(ERR_MISALIGNED_MEMORY_ACCESS);
    }
    pc = addr;
    goto jump_end;
  }
  {
  op_jal:;
    const uint8_t rd = GET_RD(instruction);
    const uint32_t imm =
        ((instruction << 11) & 0x7F800000) | ((instruction << 2) & (1 << 22)) |
        ((instruction >> 9) & 0x3FF000) | (instruction & (1 << 31));
    int32_t immi = ((int32_t)imm) >> 11;

    if (rd) {
      SET_TO_REG(rd, pc + 4);
    }
    if (immi & 0x3) {
#if USE_PRINT
      fprintf(stderr,
              "misaligned jal at pc %0X (instruction %X) address change %d\n",
              pc, instruction, immi);
#endif
      exit_loop(ERR_MISALIGNED_MEMORY_ACCESS);
    }
    pc += immi;
    goto jump_end;
  }
  {
  op_ecall:;
    // uint32_t gp = registers[3];
    /*
    // uint32_t a0 = GET_FROM_REG(10); // argument
    // uint32_t a7 = GET_FROM_REG(17); // function
    */
    uint32_t a0;
    GET_FROM_REG(a0, 10); // argument
    uint32_t a7;
    GET_FROM_REG(a7, 17); // function
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
      exit_loop(exit_code);
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
  {
  op_system:;
    const uint8_t funct3 = GET_FUNCT3(instruction);
    const uint8_t funct7 = GET_FUNCT7(instruction);
    // const uint32_t v1 = registers[GET_RS1(instruction)];
    // const uint32_t v2 = registers[GET_RS2(instruction)];

    if (funct3 == 0x0) {
      switch (funct7) {
      case 0x0: // ecall
        // return op_ecall(registers, instruction, ext_exit_code);
        goto op_ecall;
        break;
      case 0x1: // ebreak
#if USE_PRINT
        fprintf(stderr, "ebreak, exiting\n");
#endif
        exit_loop(ERR_EBREAK);
        break;
      case 0x8: // wfi
        exit_loop(ERR_WFI);
#if USE_PRINT
        fprintf(stderr, "wfi instruction, exiting\n");
#endif
        break;
      default:
        break;
      }
      goto normal_end;
    }
    const uint8_t rd = GET_RD(instruction);
    const uint32_t csr = (instruction >> 20) & 0xFFF;
#if USE_PRINT && LOG_TRACE
    const uint8_t rs1 = GET_RS1(instruction);
    printf("rd %02d rs1 %02d csr %03X\n", rd, rs1, csr);
    dbg_dump_registers_short(registers);
    printf("CSR op rd %02d rs1 0x%02d csr 0x%03X funct3 %d\n", rd, rs1, csr,
           funct3);
    printf("%s register 0x%X \n", reg_func_name[funct3], csr);
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
          SET_TO_REG(rd, 0);
        }
      } break;
      case 0x300: // mstatus (Machine status register.)
      {
        if (rd) {
          SET_TO_REG(rd, 0);
        }
      } break;
      case 0x305: // mtvec (Machine trap-handler base address.)
      {
        if (rd) {
          SET_TO_REG(rd, 0);
        }
      } break;
      case 0xb00: // mcycle (Machine cycle counter.)
      {
        if (rd) {
          SET_TO_REG(rd, get_cycles());
        }
      } break;
      case 0xB80: // mcycleh
      {
        if (rd) {
          SET_TO_REG(rd, get_cycles() >> 32);
        }
      } break;
      case 0xb02: // minstret (Machine instructions-retired counter.)
      {
        if (rd) {
          SET_TO_REG(rd, mcycle_val);
        }
      } break;
      case 0xB82: // minstreth
      {
        if (rd) {
          SET_TO_REG(rd, mcycle_val >> 32);
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
    goto normal_end;
  }
}

#if LOG_TRACE
#if USE_ZMM_REGISTERS
void dbg_dump_registers_short(uint32_t *reg) {
  for (int i = 0; i < 32; i++) {
    uint32_t val;
    GET_FROM_REG(val, i);
    if (val) {
      printf("%02d 0x%04X ", i, val);
    }
  }
  printf("\n");
}
#else
void dbg_dump_registers_short(uint32_t *reg) {
  for (int i = 0; i < 32; i++) {
    uint32_t val = reg[i];
    if (val) {
      printf("%02d 0x%04X ", i, val);
    }
  }
  printf("\n");
}
#endif
#endif
