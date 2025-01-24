

#include "riscv-vm-optimized-1.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define SYS_write 64
#define SYS_exit 93
#define SYS_access 100
#define SYS_fopen 101
#define SYS_fscanf 102
#define SYS_feof 103
#define SYS_fclose 104
#define SYS_open 105
#define SYS_fstat 106
#define SYS_read 107
#define SYS_close 108
#define SYS_lseek 109

#define MAX_FILES 128

static FILE *fopened[MAX_FILES];
static int opened[MAX_FILES];

// Static initialization function
void init_function(void) __attribute__((constructor));
void init_function(void) {
  memset(fopened, 0, sizeof(fopened));
  memset(opened, 0, sizeof(opened));
}

struct guest_stat {
  uint32_t st_size; /* [XSI] file size, in bytes */
};

uint32_t syscall_handler(uint32_t syscall_number, uint32_t arg1, uint32_t arg2,
                         uint32_t arg3, uint32_t arg4 __attribute__((unused)),
                         uint32_t arg5 __attribute__((unused)),
                         uint32_t arg6 __attribute__((unused)),
                         uint32_t arg7 __attribute__((unused)), void *wmem) {
#if LOG_TRACE
  printf("syscall_handler: syscall_number=%d, arg1=%d, arg2=%d, arg3=%d, "
         "arg4=%d, arg5=%d, arg6=%d arg7=%d\n",
         syscall_number, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
#endif
  switch (syscall_number) {
  case SYS_write: // write
  {
    if (arg1 == 1 || arg1 == 2) {
      int res = write(arg1, wmem + arg2, arg3);
      //   printf("--> write result %d\n", res);
      if (arg1) {
        fflush(stdout);
      } else {
        fflush(stderr);
      }
      //   for (int i = 0; i < (int)arg3; i++) {
      //     printf("%c", ((char *)wmem)[arg2 + i]);
      //   }
      return (uint32_t)res;
    }
  } break;
  case SYS_access: {
    char *path = (char *)wmem + arg1;
    int res = access(path, arg2);
    printf("--> access result path %s mode %d result %d\n", path, arg2, res);
    return (uint32_t)res;
  } break;
  case SYS_fopen: {
    FILE *res = fopen((char *)wmem + arg1, (char *)wmem + arg2);
    if (res == NULL) {
      printf("--> fopen '%s' result NULL\n", (char *)wmem + arg1);
      return 0;
    }
    int ind = 3;
    for (; ind < MAX_FILES && fopened[ind]; ind++)
      ;
    if (ind == MAX_FILES) {
      printf("--> no more file descriptors '%s'\n", (char *)wmem + arg1);
      return 0;
    }
    fopened[ind] = res;
    return ind;
  }
  case SYS_fscanf: {
    FILE *f = fopened[arg1];
    if (f == NULL) {
      printf("--> fscanf file descriptor %d not open\n", arg1);
      return (uint32_t)EOF;
    }
    int res = fscanf(f, (char *)wmem + arg2, (char *)wmem + arg3,
                     (char *)wmem + arg4);
    // printf("--> fscanf result %d\n", res);
    return (uint32_t)res;
  }
  case SYS_feof: {
    FILE *f = fopened[arg1];
    if (f == NULL) {
      printf("--> feof file descriptor %d not open\n", arg1);
      return (uint32_t)EOF;
    }
    int res = feof(f);
    // printf("--> feof result %d\n", res);
    return (uint32_t)res;
  }
  case SYS_fclose: {
    FILE *f = fopened[arg1];
    if (f == NULL) {
      printf("--> fclose file descriptor %d not open\n", arg1);
      return (uint32_t)EOF;
    }
    int res = fclose(f);
    // printf("--> fclose result %d\n", res);
    fopened[arg1] = NULL;
    return (uint32_t)res;
  } break;
  case SYS_open: {
    char *path = (char *)wmem + arg1;
    int res = open(path, arg2, arg3);
    // printf("--> open result path %s mode %d result %d\n", path, arg2, res);
    return (uint32_t)res;
  } break;
  case SYS_fstat: {
    struct stat host_stat;
    int res = fstat(arg1, &host_stat);
    if (res != 0) {
      printf("--> fstat fd %d result %d\n", arg1, res);
      return (uint32_t)res;
    }

    struct guest_stat *gs = (struct guest_stat *)((uint8_t *)wmem + arg2);
    gs->st_size = host_stat.st_size;
    // printf("VM: --> fstat fd %d result %d size %lld\n", arg1, res, host_stat.st_size);
    return (uint32_t)res;
  } break;
  case SYS_read: {
    int res = read(arg1, wmem + arg2, arg3);
    // printf("--> read result %d\n", res);
    return (uint32_t)res;
  } break;
  case SYS_close: {
    int res = close(arg1);
    // printf("--> close result %d\n", res);
    return (uint32_t)res;
  } break;
  case SYS_lseek: {
    int res = lseek(arg1, (off_t)arg2, arg3);
    // printf("HOST --> lseek result %d\n", res);
    return (uint32_t)res;
  } break;

  default:
    break;
  }

  return 0;
}
