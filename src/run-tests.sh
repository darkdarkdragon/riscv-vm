#!/bin/bash
make runelf

# Define the target directory
TARGET_DIR="../compiled-tests"

echo "Running RV32UI tests"
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
    ./runelf "$file" -opt2
    
    # Check the exit status of `runelf`
    if [[ $? -ne 0 ]]; then
        echo "Execution failed for file: $file"
        exit 1
    fi
done

echo "All RV32UI tests passed."

echo "Running RV32UM tests"
for file in "$TARGET_DIR"/rv32um-p-*; do
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
    ./runelf "$file" -opt2
    
    # Check the exit status of `runelf`
    if [[ $? -ne 0 ]]; then
        echo "Execution failed for file: $file"
        exit 1
    fi
done

echo "All RV32UM tests passed."
