# RISC-V VM

Implements RV32I with M extension.

To build VM itself only GCC C complier needed.
Build command `make` will produce executable `runelf`.
This executable runs `.elf` produced by `riscv32-unknown-elf-gcc`.

There is two versions - one in `riscv-vm-portable.c` and one in `riscv-vm-optimized-1.c`.
`riscv-vm-portable.c` is first one I wrote without any optimization, goal was to make it pass tests.
I kept it for comparison, to check that my optimizations really do something.

`riscv-vm-optimized-1.c` - optimized version, though it doesn't use anything specific to any CPU architecture.

I've used https://github.com/riscv/riscv-tests tests to check correctness of my implementation.
Relevant compiled tests are in `compiled-tests` directory.
To run test run `run-tests.sh`.

I've adopted two benchmarks from https://github.com/riscv-software-src/riscv-tests/tree/master/benchmarks and wrote one own.
Source and compiled binaries for benchmarks are in `benchmarks` directory.

For each bencmark there is separate target in Makefile.
I was running benchmarks like this:
`time make run-towers`
I've used won Macbook and `t3.micro` AWS instance to run benchmarks.
Results are in `benchmarks_results.md`.
Also I've ran these benchmarks against [Spike](https://github.com/riscv-software-src/riscv-isa-sim) which is reference RISCV simulator.

## Implementation notes

VM allocates 16MiB - I've choose so to accomodate `median` benchmark that uses larger amount of memory.

`riscv_vm_run_optimized_1` functiion takes as argument initial registers values and program memory,
but I've not used initial registers anywhere in my tests because all my tests are compiled by GCC and it emits code to zero out
registers at start.

I've implented `ecall` as used by `https://github.com/riscv/riscv-tests` to print to console and to exit program.


## RISCV toolchain

Tests and benchmarks was compiled using RISCV toolchain built from source like this:

```sh
git clone https://github.com/riscv/riscv-gnu-toolchain

cd riscv-gnu-toolchain
./configure --prefix=/opt/riscv --enable-multilib --with-isa-spec=2.2
make
```
