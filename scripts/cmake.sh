#!/bin/sh

set -eux -o pipefail

PRESET="${1:-Release}"

# Generate build system specified in build directory with cmake
cmake --preset "$PRESET"

# Build the underlying build system via CMake
cmake --build --preset "$PRESET"

# Run the tests
ctest --preset "$PRESET"
