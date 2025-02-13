#!/bin/sh

set -eux -o pipefail

# Make a build directory
mkdir -p build

# Generate build system specified in build directory with cmake
cmake -Bbuild -GNinja -DCMAKE_EXPORT_COMPILE_COMMANDS=1

# Build the underlying build system via CMake
cmake --build build

# Run the tests
ctest --test-dir build
