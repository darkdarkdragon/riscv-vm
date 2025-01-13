// Most of this file generated by Claude

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "riscv-vm-optimized-1.h"
#include "riscv-vm-portable.h"

/* ELF Definitions */
#define EI_NIDENT 16
#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'
#define ELFMAG "\177ELF"
#define SELFMAG 4

#define EI_CLASS 4
#define ELFCLASSNONE 0
#define ELFCLASS32 1
#define ELFCLASS64 2

#define EI_DATA 5
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

#define EI_VERSION 6

/* Machine types */
#define EM_NONE 0
#define EM_386 3
#define EM_X86_64 62
#define EM_RISCV 243

/* Section types */
#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_HASH 5
#define SHT_DYNAMIC 6
#define SHT_NOTE 7
#define SHT_NOBITS 8
#define SHT_REL 9
#define SHT_SHLIB 10
#define SHT_DYNSYM 11

/* Section flags */
#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2
#define SHF_EXECINSTR 0x4
#define SHF_MERGE 0x10
#define SHF_STRINGS 0x20
#define SHF_INFO_LINK 0x40

/* Program header types */
#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6
#define PT_TLS 7

/* Program header flags */
#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

/* ELF Header */
typedef struct {
  unsigned char e_ident[EI_NIDENT];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint32_t e_entry;
  uint32_t e_phoff;
  uint32_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
} Elf32_Ehdr;

/* Program header */
typedef struct {
  uint32_t p_type;
  uint32_t p_offset;
  uint32_t p_vaddr;
  uint32_t p_paddr;
  uint32_t p_filesz;
  uint32_t p_memsz;
  uint32_t p_flags;
  uint32_t p_align;
} Elf32_Phdr;

typedef struct {
  unsigned char e_ident[EI_NIDENT];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint64_t e_entry;
  uint64_t e_phoff;
  uint64_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
} Elf64_Ehdr;

/* Section header */
typedef struct {
  uint32_t sh_name;
  uint32_t sh_type;
  uint32_t sh_flags;
  uint32_t sh_addr;
  uint32_t sh_offset;
  uint32_t sh_size;
  uint32_t sh_link;
  uint32_t sh_info;
  uint32_t sh_addralign;
  uint32_t sh_entsize;
} Elf32_Shdr;

typedef struct {
  uint32_t sh_name;
  uint32_t sh_type;
  uint64_t sh_flags;
  uint64_t sh_addr;
  uint64_t sh_offset;
  uint64_t sh_size;
  uint32_t sh_link;
  uint32_t sh_info;
  uint64_t sh_addralign;
  uint64_t sh_entsize;
} Elf64_Shdr;

void print_section_flags(uint64_t flags) {
  printf("Flags: ");
  if (flags & SHF_WRITE)
    printf("WRITE ");
  if (flags & SHF_ALLOC)
    printf("ALLOC ");
  if (flags & SHF_EXECINSTR)
    printf("EXEC ");
  if (flags & SHF_MERGE)
    printf("MERGE ");
  if (flags & SHF_STRINGS)
    printf("STRINGS ");
  if (flags & SHF_INFO_LINK)
    printf("INFO_LINK ");
  printf("\n");
}

char *get_section_type(uint32_t sh_type) {
  switch (sh_type) {
  case SHT_NULL:
    return "NULL";
  case SHT_PROGBITS:
    return "PROGBITS";
  case SHT_SYMTAB:
    return "SYMTAB";
  case SHT_STRTAB:
    return "STRTAB";
  case SHT_RELA:
    return "RELA";
  case SHT_HASH:
    return "HASH";
  case SHT_DYNAMIC:
    return "DYNAMIC";
  case SHT_NOTE:
    return "NOTE";
  case SHT_NOBITS:
    return "NOBITS";
  case SHT_REL:
    return "REL";
  case SHT_SHLIB:
    return "SHLIB";
  case SHT_DYNSYM:
    return "DYNSYM";
  default:
    return "UNKNOWN";
  }
}

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

void print_elf_header_info(unsigned char *e_ident) {
  printf("ELF Header Information:\n");
  printf("Magic: %02x %02x %02x %02x\n", e_ident[0], e_ident[1], e_ident[2],
         e_ident[3]);
  printf("Class: %s\n", e_ident[EI_CLASS] == ELFCLASS32 ? "32-bit" : "64-bit");
  printf("Data: %s\n",
         e_ident[EI_DATA] == ELFDATA2LSB ? "Little-endian" : "Big-endian");
  printf("Version: %d\n", e_ident[EI_VERSION]);
  printf("\n");
}

char *get_segment_type(uint32_t p_type) {
  switch (p_type) {
  case PT_NULL:
    return "NULL";
  case PT_LOAD:
    return "LOAD";
  case PT_DYNAMIC:
    return "DYNAMIC";
  case PT_INTERP:
    return "INTERP";
  case PT_NOTE:
    return "NOTE";
  case PT_SHLIB:
    return "SHLIB";
  case PT_PHDR:
    return "PHDR";
  case PT_TLS:
    return "TLS";
  default:
    return "UNKNOWN";
  }
}

void print_segment_flags(uint32_t flags) {
  printf("Flags: ");
  if (flags & PF_R)
    printf("R");
  if (flags & PF_W)
    printf("W");
  if (flags & PF_X)
    printf("X");
  printf("\n");
}

/* Rest of the code remains the same as in previous version */
void process_elf32(void *file_data) {
  Elf32_Ehdr *ehdr = (Elf32_Ehdr *)file_data;
  Elf32_Phdr *phdr = (Elf32_Phdr *)((char *)file_data + ehdr->e_phoff);
  Elf32_Shdr *shdr = (Elf32_Shdr *)((char *)file_data + ehdr->e_shoff);
  char *strtab = (char *)file_data + shdr[ehdr->e_shstrndx].sh_offset;

  printf("Architecture: %s\n", get_machine_name(ehdr->e_machine));
  printf("Number of sections: %d\n", ehdr->e_shnum);
  printf("Entry: 0x%x\n\n", ehdr->e_entry);

  // Print Program Headers
  printf("\nProgram Headers (%d):\n", ehdr->e_phnum);
  for (int i = 0; i < ehdr->e_phnum; i++) {
    printf("\nProgram Header %d:\n", i);
    printf("Type: %s\n", get_segment_type(phdr[i].p_type));
    printf("Offset: 0x%x\n", phdr[i].p_offset);
    printf("VirtAddr: 0x%x\n", phdr[i].p_vaddr);
    printf("PhysAddr: 0x%x\n", phdr[i].p_paddr);
    printf("FileSize: 0x%x\n", phdr[i].p_filesz);
    printf("MemSize: 0x%x\n", phdr[i].p_memsz);
    print_segment_flags(phdr[i].p_flags);
    printf("Align: 0x%x\n", phdr[i].p_align);
  }

  // Print Section Headers
  printf("\nSection Headers (%d):\n", ehdr->e_shnum);

  for (int i = 0; i < ehdr->e_shnum; i++) {
    printf("Section %2d: %s\n", i, &strtab[shdr[i].sh_name]);
    printf("Type: %s\n", get_section_type(shdr[i].sh_type));
    printf("Address: 0x%x\n", shdr[i].sh_addr);
    printf("Offset: 0x%x\n", shdr[i].sh_offset);
    printf("Size: %u bytes\n", shdr[i].sh_size);
    print_section_flags(shdr[i].sh_flags);
    printf("\n");
  }
}

void run_elf32(void *file_data) {
  Elf32_Ehdr *ehdr = (Elf32_Ehdr *)file_data;
  Elf32_Shdr *shdr = (Elf32_Shdr *)((char *)file_data + ehdr->e_shoff);
  char *strtab = (char *)file_data + shdr[ehdr->e_shstrndx].sh_offset;

  // printf("Architecture: %s\n", get_machine_name(ehdr->e_machine));
  // printf("Number of sections: %d\n\n", ehdr->e_shnum);
  uint8_t *data = 0;
  uint32_t data_len = 0;
  uint32_t data_offset = 0;
  for (int i = 0; i < ehdr->e_shnum; i++) {
    if (strcmp(&strtab[shdr[i].sh_name], ".data") == 0) {
      data = (uint8_t *)((char *)file_data + shdr[i].sh_offset);
      data_len = shdr[i].sh_size;
      data_offset = shdr[i].sh_addr - ehdr->e_entry;
      printf(
          "Found .data section entry 0x%X data address 0x%X data_offset 0x%X\n",
          ehdr->e_entry, shdr[i].sh_addr, data_offset);
      break;
    }
  }

  for (int i = 0; i < ehdr->e_shnum; i++) {
    // printf("Section %2d: %s\n", i, &strtab[shdr[i].sh_name]);
    // printf("Type: %s\n", get_section_type(shdr[i].sh_type));
    // printf("Address: 0x%x\n", shdr[i].sh_addr);
    // printf("Offset: 0x%x\n", shdr[i].sh_offset);
    // printf("Size: %u bytes\n", shdr[i].sh_size);
    // print_section_flags(shdr[i].sh_flags);
    // printf("\n");
    if (strcmp(&strtab[shdr[i].sh_name], ".text.init") == 0) {
      printf("Found .text.init section\n");
      uint8_t *text = (uint8_t *)((char *)file_data + shdr[i].sh_offset);
      riscv_vm_run(NULL, text, shdr[i].sh_size, data, data_len, data_offset);
      break;
    }
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

void process_elf64(void *file_data) {
  Elf64_Ehdr *ehdr = (Elf64_Ehdr *)file_data;
  Elf64_Shdr *shdr = (Elf64_Shdr *)((char *)file_data + ehdr->e_shoff);
  char *strtab = (char *)file_data + shdr[ehdr->e_shstrndx].sh_offset;

  printf("Architecture: %s\n", get_machine_name(ehdr->e_machine));
  printf("Number of sections: %d\n\n", ehdr->e_shnum);

  for (int i = 0; i < ehdr->e_shnum; i++) {
    printf("Section %2d: %s\n", i, &strtab[shdr[i].sh_name]);
    printf("Type: %s\n", get_section_type(shdr[i].sh_type));
    printf("Address: 0x%" PRIx64 "\n", shdr[i].sh_addr);
    printf("Offset: 0x%" PRIx64 "\n", shdr[i].sh_offset);
    printf("Size: %" PRIu64 " bytes\n", shdr[i].sh_size);
    print_section_flags(shdr[i].sh_flags);
    printf("\n");
  }
}

int main(int argc, char *argv[]) {
  int use_optimized = 0;
  int verbose = 0;
  int file_index = 1;
  if (argc < 2 || argc > 4) {
    fprintf(stderr, "Usage: [-opt] [-verbose] %s <elf-file>\n", argv[0]);
    return 1;
  }
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-opt") == 0) {
      use_optimized = 1;
    } else if (strcmp(argv[i], "-opt2") == 0) {
      use_optimized = 2;
    } else if (strcmp(argv[i], "-opt3") == 0) {
      use_optimized = 3;
    } else if (strcmp(argv[i], "-opt4") == 0) {
      use_optimized = 4;
    } else if (strcmp(argv[i], "-verbose") == 0) {
      verbose = 1;
    } else {
      file_index = i;
    }
  }
  printf("Loading file %s\n", argv[file_index]);
  int fd = open(argv[file_index], O_RDONLY);
  if (fd < 0) {
    perror("Error opening file");
    return 1;
  }

  struct stat st;
  if (fstat(fd, &st) < 0) {
    perror("Error getting file size");
    close(fd);
    return 1;
  }

  void *file_data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (file_data == MAP_FAILED) {
    perror("Error mapping file");
    close(fd);
    return 1;
  }

  unsigned char *e_ident = (unsigned char *)file_data;
  if (memcmp(e_ident, ELFMAG, SELFMAG) != 0) {
    fprintf(stderr, "Not an ELF file\n");
    munmap(file_data, st.st_size);
    close(fd);
    return 1;
  }

  if (verbose) {
    print_elf_header_info(e_ident);
  }

  int exit_code = 0;
  if (e_ident[EI_CLASS] == ELFCLASS32) {
    // process_elf32(file_data);
    // run_elf32(file_data);
    exit_code = run_elf32v2(file_data, verbose, use_optimized);
  } else if (e_ident[EI_CLASS] == ELFCLASS64) {
    // process_elf64(file_data);
    fprintf(stderr, "Can't run 64 bit programs\n");
  } else {
    fprintf(stderr, "Unsupported ELF class\n");
  }

  munmap(file_data, st.st_size);
  close(fd);
  return exit_code;
}
