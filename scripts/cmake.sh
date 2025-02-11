#!/bin/sh

set -eux -o pipefail

# Make a build directory
mkdir -p build

# Generate build system specified in build directory with cmake
cmake -Bbuild -GNinja

# Build the underlying build system via CMake
cmake --build build

# Run the tests
ctest --test-dir build
