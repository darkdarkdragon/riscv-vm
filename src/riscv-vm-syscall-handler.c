

#include "riscv-vm-optimized-1.h"
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

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
  case 64: // write
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
  default:
    break;
  }

  return 0;
}
