#!/bin/bash

# Format all C and header files in the project using clang-format
# Excludes build directories and other generated files

set -e

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format is not installed."
    echo "Install it with: brew install clang-format"
    exit 1
fi

echo "Formatting C/C++ files with clang-format..."

# Find and format all C/C++ files, excluding build directories
find . \( -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" \) \
    -not -path "./build*" \
    -not -path "./managed_components/*" \
    -not -path "./.git/*" \
    -exec clang-format -i {} \;

echo "✅ All C/C++ files have been formatted!"