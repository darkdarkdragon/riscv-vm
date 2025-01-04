#!env bash

echo "Running tests"

#!/bin/bash

# Define the target directory
TARGET_DIR="../mess/riscv-tests/isa"

# Find all files matching the pattern without extensions
for file in "$TARGET_DIR"/rv32ui-p-*; do
    # Check if the current item is a file without an extension
    base_name=$(basename "$file")
    # echo "$base_name"
    if [[ "$base_name" == *.* ]]; then
        echo "Skipping file: $file"
        continue  # Skip files with extensions
    fi
    # Print the file being executed
    echo "Executing file: $file"
    
    # Run the `runelf` program with the current file
    ./runelf "$file"
    
    # Check the exit status of `runelf`
    if [[ $? -ne 0 ]]; then
        echo "Execution failed for file: $file"
        exit 1
    fi
done

echo "All files executed successfully."


# # Define the target directory
# TARGET_DIR="../mess/riscv-tests/isa"

# # Find all files starting with 'rv32ui-p-' in the target directory
# for file in "$TARGET_DIR"/rv32ui-p-*; do
#     # Check if any files were found
#     if [[ ! -e "$file" ]]; then
#         echo "No matching files found."
#         exit 1
#     fi

#     # Print the file being executed
#     echo "Executing file: $file"

#     # Run the `runelf` program with the current file
#     ./runelf "$file"
    
#     # Check the exit status of `runelf`
#     if [[ $? -ne 0 ]]; then
#         echo "Execution failed for file: $file"
#         exit 1
#     fi
# done

# echo "All files executed successfully."

