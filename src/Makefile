
# Compiler
CC = gcc

# Compiler flags
#CFLAGS = -Wall -Wextra -O2
CFLAGS = 

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
SRC_RUNELF=runelf.c riscv-vm-portable.c

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
	./$(OUT_RUNELF) rv32ui-p-addi

# Clean target
clean:
	rm -f $(OUT_SIMPLE) $(OUT_RUNELF)

.PHONY: all clean run-simple run-runelf

