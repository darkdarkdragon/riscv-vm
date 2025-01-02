
# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -O2

# Target executable name
TARGET = simple

# Source files
SRCS = simple.c riscv-vm-portable.c

# Object files (replace .c with .o in SRCS)
OBJS = $(SRCS:.c=.o)

# Source file
#SRC = simple.c risc-vm-portable.c

# Build the program
all: $(TARGET)

# $(TARGET): $(SRC)
# 	$(CC) -o $(TARGET) $(SRC)

# Rule to build the target
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Rule to build object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up generated files
# clean:
# 	rm -f $(TARGET)
# Clean rule
clean:
	rm -f $(OBJS) $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean
