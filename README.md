# RISC-V VM

Implements RV32I with M extension.



## RISCV toolchain

Tests and benchmarks was compiled using RISCV toolchain built from source like this:

```sh
git clone https://github.com/riscv/riscv-gnu-toolchain

cd riscv-gnu-toolchain
./configure --prefix=/opt/riscv --enable-multilib --with-isa-spec=2.2
make
```
