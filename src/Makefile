
# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -O3
# CFLAGS = 

# Target executable name
# TARGET = simple runelf

# Source files
# SRCS = simple.c riscv-vm-portable.c runelf.c

# Object files (replace .c with .o in SRCS)
# OBJS = $(SRCS:.c=.o)

# Source file
#SRC = simple.c risc-vm-portable.c

# Build the program
# all: $(TARGET)

# $(TARGET): $(SRC)
# 	$(CC) -o $(TARGET) $(SRC)

# Rule to build the target
# $(TARGET): $(OBJS)
# 	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Rule to build object files
# %.o: %.c
# 	$(CC) $(CFLAGS) -c $< -o $@

# Clean up generated files
# clean:
# 	rm -f $(TARGET)
# Clean rule
# clean:
# 	rm -f $(OBJS) $(TARGET)

# Run the program
# run: $(TARGET)
# 	./$(TARGET)

# .PHONY: all clean

# Source files
SRC_SIMPLE=simple.c riscv-vm-portable.c
SRC_RUNELF=runelf.c riscv-vm-portable.c riscv-vm-optimized-1.c

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

run-runelf: $(OUT_RUNELF)
#	./$(OUT_RUNELF) towers070.riscv -verbose -opt
#	./$(OUT_RUNELF) towers028.riscv -verbose -opt
	./$(OUT_RUNELF) median1m.riscv -verbose 
#	 ./$(OUT_RUNELF) rv32ui-p-lh
#	 ./$(OUT_RUNELF) dhrystone.riscv -opt
#	 ./$(OUT_RUNELF) 	 qsort.riscv
#	 ./$(OUT_RUNELF) memcpy.riscv
#	 ./$(OUT_RUNELF) multiply.riscv

run-towers: $(OUT_RUNELF)
	./$(OUT_RUNELF) towers028.riscv -verbose -opt

spike-towers: $(OUT_RUNELF)
	spike --isa=rv32gcv towers028.riscv

run-median: $(OUT_RUNELF)
	./$(OUT_RUNELF)  ../benchmarks/median.riscv -verbose -opt

spike-median: $(OUT_RUNELF)
	spike --isa=rv32im ../benchmarks/median.riscv

run-prime: $(OUT_RUNELF)
	./$(OUT_RUNELF)  ../benchmarks/prime.riscv -verbose -opt

spike-prime: $(OUT_RUNELF)
	spike --isa=rv32im ../benchmarks/prime.riscv

# Clean target
clean:
	rm -f $(OUT_SIMPLE) $(OUT_RUNELF)

.PHONY: all clean run-simple run-runelf

