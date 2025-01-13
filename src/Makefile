
# Compiler
CC = gcc

# Compiler flags
#CFLAGS = -Wall -Wextra -O3 -mavx512f -march=skylake-avx512  -mno-sse
#CFLAGS = -Wall -Wextra -O3 -mavx512f -march=skylake-avx512  
#CFLAGS = -Wall -Wextra -O3 -mavx512f -march=skylake-avx512  
#CFLAGS = -Wall -Wextra -O3 -msse3
#CFLAGS = -Wall -Wextra -O3 -mno-sse
CFLAGS = -Wall -Wextra -O3
#CFLAGS = 


# Source files
SRC_SIMPLE=simple.c riscv-vm-portable.c
SRC_RUNELF=runelf.c riscv-vm-portable.c riscv-vm-optimized-1.c riscv-vm-optimized-2.c riscv-vm-common.c riscv-vm-optimized-3.c riscv-vm-optimized-4.c

# Output executables
OUT_SIMPLE=simple
OUT_RUNELF=runelf

# Default target
all: $(OUT_SIMPLE) $(OUT_RUNELF)

$(OUT_SIMPLE): $(SRC_SIMPLE)
	$(CC) $(CFLAGS) -o $@ $^

$(OUT_RUNELF): $(SRC_RUNELF)
	$(CC) $(CFLAGS) -o $@ $^

# Run targets
run-simple: $(OUT_SIMPLE)
	./$(OUT_SIMPLE)

run-mul: clean $(OUT_RUNELF)
	./$(OUT_RUNELF) ../compiled-tests/rv32um-p-mul -opt2

run-div: clean $(OUT_RUNELF)
	./$(OUT_RUNELF) ../compiled-tests/rv32um-p-div -opt4

run-runelf: $(OUT_RUNELF)
	./$(OUT_RUNELF) median1m.riscv -verbose 

run-towers-port: $(OUT_RUNELF)
	./$(OUT_RUNELF) ../benchmarks/towers.riscv

run-towers: $(OUT_RUNELF)
	./$(OUT_RUNELF) ../benchmarks/towers.riscv -verbose -opt2

run-towers1: $(OUT_RUNELF)
	./$(OUT_RUNELF) ../benchmarks/towers.riscv -verbose -opt

spike-towers: $(OUT_RUNELF)
	spike --isa=rv32im ../benchmarks/towers.riscv

run-median-port: $(OUT_RUNELF)
	./$(OUT_RUNELF)  ../benchmarks/median.riscv

run-median: $(OUT_RUNELF)
	./$(OUT_RUNELF)  ../benchmarks/median.riscv -verbose -opt2

run-median1: $(OUT_RUNELF)
	./$(OUT_RUNELF)  ../benchmarks/median.riscv -verbose -opt

spike-median: $(OUT_RUNELF)
	spike --isa=rv32im ../benchmarks/median.riscv

run-prime-port: $(OUT_RUNELF)
	./$(OUT_RUNELF)  ../benchmarks/prime.riscv 

run-prime: $(OUT_RUNELF)
	./$(OUT_RUNELF)  ../benchmarks/prime4.riscv -verbose -opt4

run-prime1: $(OUT_RUNELF)
	./$(OUT_RUNELF)  ../benchmarks/prime4.riscv -verbose -opt

run-prime2: $(OUT_RUNELF)
	./$(OUT_RUNELF)  ../benchmarks/prime.riscv -verbose -opt2

spike-prime: $(OUT_RUNELF)
	spike --isa=rv32im ../benchmarks/prime.riscv

# Clean target
clean:
	rm -f $(OUT_SIMPLE) $(OUT_RUNELF)

rebuild: clean all

retest: rebuild
	./$(OUT_RUNELF) rv32ui-p-lui -opt

.PHONY: all clean run-simple run-runelf retest rebuild
