
#include <inttypes.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define APP_IMPLEMENTATION
// #define  APP_WINDOWS
#define APP_SDL
#define APP_S16 int16_t
#define APP_U32 uint32_t
#define APP_U64 uint64_t
#include "app.h"

#include <stdlib.h> // for rand and __argc/__argv
#include <string.h> // for memset

#include "riscv-vm-portable.h"
#include "runelf-lib.h"

static void *file_data;

/*
int app_proc(app_t *app, void *user_data) {
  APP_U32 canvas[320 * 200];            // a place for us to draw stuff
  memset(canvas, 0xC0, sizeof(canvas)); // clear to grey
  app_screenmode(app, APP_SCREENMODE_WINDOW);

  // keep running until the user close the window
  while (app_yield(app) != APP_STATE_EXIT_REQUESTED) {
    // plot a random pixel on the canvas
    int x = rand() % 320;
    int y = rand() % 200;
    APP_U32 color = rand() | ((APP_U32)rand() << 16);
    canvas[x + y * 320] = color;

    // display the canvas
    app_present(app, canvas, 320, 200, 0xffffff, 0x000000);
  }
  return 0;
}
*/
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

static APP_U32 canvas[SCREEN_WIDTH * SCREEN_HEIGHT]; // a place for us to draw stuff
static APP_U32 palette[256];
static app_t *g_app;

#define SYS_present_screen 1024
#define SYS_set_palette 1025

static uint32_t graph_syscall_handler(uint32_t *handled, uint32_t syscall_number, uint32_t arg1, uint32_t arg2, uint32_t arg3,
                                      uint32_t arg4, uint32_t arg5, uint32_t arg6, uint32_t arg7, void *wmem) {

  switch (syscall_number) {
  case SYS_set_palette: {
    // printf("SYS_set_paletter\n");
    *handled = 1;
    // int index = arg1;
    // APP_U32 color = arg2;
    palette[arg1] = arg2 | 0xff000000;
    return 0;
  } break;
  case SYS_present_screen: {
    // printf("SYS_present_screen\n");
    *handled = 1;
    // int x = arg1;
    // int y = arg2;
    // APP_U32 color = arg3;
    // canvas[x + y * 320] = color;
    uint8_t *src = wmem + arg1;
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
      for (int x = 0; x < SCREEN_WIDTH; x++) {
        canvas[x + y * SCREEN_WIDTH] = palette[src[x + y * SCREEN_WIDTH]] | 0xff000000;
      }
    }
    if (app_yield(g_app) == APP_STATE_EXIT_REQUESTED) {
      *handled = 2;
      return 0;
    }
    app_present(g_app, canvas, SCREEN_WIDTH, SCREEN_HEIGHT, 0xffffff, 0x000000);
    return 0;
  } break;
  default:
    break;
  }

  return 0;
}

int app_proc(app_t *app, void *user_data) {
  g_app = app;
  memset(canvas, 0xC0, sizeof(canvas)); // clear to grey
  for (int i = 0; i < 256; i++) {
    palette[i] = i | (i << 8) | (i << 16) | 0xff000000;
  }
  app_screenmode(app, APP_SCREENMODE_WINDOW);
  int exit_code = run_elf32v2(file_data, 0, 4, graph_syscall_handler);
  // int exit_code = run_elf32v2(file_data, 0, 4, 0);
  return exit_code;
}

int main(int argc, char **argv) {
  (void)argc, argv;

  int verbose = 0;
  int file_index = 1;
  if (argc < 2 || argc > 4) {
    fprintf(stderr, "Usage: [-verbose] %s <elf-file>\n", argv[0]);
    return 1;
  }

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-verbose") == 0) {
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

  file_data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
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
    // print_elf_header_info(e_ident);
  }

  int exit_code = 0;
  if (e_ident[EI_CLASS] == ELFCLASS32) {
    // exit_code = run_elf32v2(file_data, verbose, 4);
    // exit_code = run_elf32v2(file_data, 0, 4, graph_syscall_handler);
    // exit_code = run_elf32v2(file_data, 0, 4, 0);
    exit_code = app_run(app_proc, NULL, NULL, NULL, NULL);
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

// pass-through so the program will build with either /SUBSYSTEM:WINDOWS or
// /SUBSYSTEN:CONSOLE extern "C" int __stdcall WinMain( struct HINSTANCE__*,
// struct HINSTANCE__*, char*, int ) { return main( __argc, __argv ); }
