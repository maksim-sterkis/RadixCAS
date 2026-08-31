#!/bin/bash

# Ensure build is up-to-date
echo "Compiling codebase..."
cmake -B build -S . && cmake --build build
if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi
echo "Build successful."
echo "----------------------------------------"

# Run tests
echo "Running main test suite (test_script.txt)..."
./build/calc < tests/test_script.txt > tests/test_script_actual.txt
echo "Test finished. Output saved to tests/test_script_actual.txt"

echo "----------------------------------------"
echo "Running secondary tests (test_input.txt)..."
./build/calc < tests/test_input.txt > tests/test_input_actual.txt
echo "Test finished. Output saved to tests/test_input_actual.txt"
echo "----------------------------------------"

echo "All tests executed successfully."
