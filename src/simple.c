
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "riscv-vm-portable.h"

int main() {
    printf("Simple\n");
    // Open the binary file for reading
    FILE *file = fopen("hello-uart.bin", "rb");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }
    // Determine the file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0) {
        fprintf(stderr, "Error: file is empty or size cannot be determined.\n");
        fclose(file);
        return 1;
    }

    // Allocate memory to hold the file content
    uint8_t *buffer = (uint8_t *)malloc(file_size);
    if (buffer == NULL) {
        fprintf(stderr, "Error allocating memory.\n");
        fclose(file);
        return 1;
    }

    // Read the file content into memory
    size_t read_size = fread(buffer, 1, file_size, file);
    if ((long)read_size != file_size) {
        fprintf(stderr, "Error reading file.\n");
        free(buffer);
        fclose(file);
        return 1;
    }

    // Close the file
    fclose(file);

    return riscv_vm_run(NULL, buffer, file_size, NULL, 0, 0);
}

