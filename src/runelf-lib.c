
#include "runelf-lib.h"
#include "riscv-vm-optimized-1.h"
#include "riscv-vm-portable.h"
#include <stdio.h>

char *get_machine_name(uint16_t machine) {
  switch (machine) {
  case EM_RISCV:
    return "RISC-V";
  case EM_X86_64:
    return "x86_64";
  case EM_386:
    return "x86";
  default:
    return "Unknown";
  }
}

int run_elf32v2(void *file_data, int verbose, int use_optimized) {
  Elf32_Ehdr *ehdr = (Elf32_Ehdr *)file_data;
  Elf32_Phdr *phdr = (Elf32_Phdr *)((char *)file_data + ehdr->e_phoff);
  // Elf32_Shdr *shdr = (Elf32_Shdr *)((char *)file_data + ehdr->e_shoff);
  // char *strtab = (char *)file_data + shdr[ehdr->e_shstrndx].sh_offset;
  uint8_t *text = 0;
  uint32_t text_len = 0;
  if (verbose) {
    printf("Architecture: %s\n", get_machine_name(ehdr->e_machine));
    printf("Number of sections: %d\n", ehdr->e_shnum);
    printf("Entry: 0x%x\n\n", ehdr->e_entry);

    // Print Program Headers
    printf("\nProgram Headers (%d):\n", ehdr->e_phnum);
  }
  for (int i = 0; i < ehdr->e_phnum; i++) {
    if (phdr[i].p_type == PT_LOAD) {
      if (text == 0) {
        text = (uint8_t *)((char *)file_data + phdr[i].p_offset);
        text_len = phdr[i].p_filesz;
        // printf("Found .text section\n");
      } else {
        uint8_t *sect_text = (uint8_t *)((char *)file_data + phdr[i].p_offset);
        sect_text += phdr[i].p_filesz;
        text_len = sect_text - text;
        // printf("Found .data section\n");
      }
    }
  }
  if (use_optimized == 1) {
    return riscv_vm_run_optimized_1(NULL, text, text_len);
  } else if (use_optimized == 2) {
    return riscv_vm_run_optimized_2(NULL, text, text_len);
  } else if (use_optimized == 3) {
    return riscv_vm_run_optimized_3(NULL, text, text_len);
  } else if (use_optimized == 4) {
    return riscv_vm_run_optimized_4(NULL, text, text_len);
  }
  return riscv_vm_run(NULL, text, text_len, 0, 0, 0);
}