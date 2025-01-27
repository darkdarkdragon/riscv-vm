
CURRENT_DIR := $(shell pwd)

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
CFLAGS_GR = $(shell pkg-config --cflags --libs glew sdl2)


# Source files
SRC_SIMPLE=simple.c riscv-vm-portable.c
SRC_RUNELF=runelf.c riscv-vm-portable.c riscv-vm-optimized-1.c riscv-vm-optimized-2.c \
  riscv-vm-common.c riscv-vm-optimized-3.c riscv-vm-optimized-4.c riscv-vm-syscall-handler.c \
  runelf-lib.c
SRC_RUNELF_GR=runelf-graph.c riscv-vm-portable.c riscv-vm-optimized-1.c riscv-vm-optimized-2.c \
  riscv-vm-common.c riscv-vm-optimized-3.c riscv-vm-optimized-4.c riscv-vm-syscall-handler.c \
  runelf-lib.c

# Output executables
OUT_SIMPLE=simple
OUT_RUNELF=runelf
OUT_RUNELF_GR=runelf-gr

# Default target
all: $(OUT_SIMPLE) $(OUT_RUNELF) $(OUT_RUNELF_GR)

$(OUT_SIMPLE): $(SRC_SIMPLE)
	$(CC) $(CFLAGS) -o $@ $^

$(OUT_RUNELF): $(SRC_RUNELF)
	$(CC) $(CFLAGS) -o $@ $^

$(OUT_RUNELF_GR): $(SRC_RUNELF_GR)
	$(CC) $(CFLAGS) $(CFLAGS_GR) -framework OpenGL -o $@ $^

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

run-doom: $(OUT_RUNELF)
	echo $(CURRENT_DIR)
	cd /Users/dark/projects/doom/riscv-vm-doom/test && $(CURRENT_DIR)/$(OUT_RUNELF) doom -verbose -opt4

run-doom-gr: $(OUT_RUNELF)
	echo $(CURRENT_DIR)
	cd /Users/dark/projects/doom/riscv-vm-doom/test && $(CURRENT_DIR)/$(OUT_RUNELF_GR) doom -verbose

# Clean target
clean:
	rm *.o || true
	rm -f $(OUT_SIMPLE) $(OUT_RUNELF) $(OUT_RUNELF_GR)

rebuild: clean all

retest: rebuild
	./$(OUT_RUNELF) rv32ui-p-lui -opt

.PHONY: all clean run-simple run-runelf retest rebuild
