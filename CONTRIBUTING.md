# Contributing Guide

## Main Technologies

- **Language**: C++ (C++14 and C++20)
- **Build Systems**: CMake and Bazel
- **Tools**: `uv` (for Python virtual environments), `pre-commit`,
  `clang-format` (v17), GitHub Actions for CI.

## Using pre-commit for git hooks

This repository uses the Python `pre-commit` library to manage git hooks run as
part of the commit process.

Install the `uv` tool if you don't have it already:
<https://docs.astral.sh/uv/getting-started/installation>.

Install pre-commit as a pre-commit hook:

```bash
uv run pre-commit install
```

## Building and testing

Tests can be built and run using either CMake or Bazel. Both build systems are
kept in sync manually.

### Building with CMake

To build and test using CMake via the provided script:

```bash
./scripts/cmake.sh
```

Or manually:

```bash
mkdir build
cmake --list-presets
cmake --preset <preset>
cmake --build --preset <preset>
ctest --preset <preset>
```

### Building with Bazel

To build and test using Bazel via the provided script:

```bash
./scripts/bazel.sh
```

Or manually:

```bash
bazel build //...
bazel test //...
```

## Including value_types in your own project

To use the value types code in your own CMake project, use `FetchContent`:

```txt
FetchContent_Declare(
    value_types
    GIT_REPOSITORY https://github.com/jbcoe/value_types
)
FetchContent_MakeAvailable(value_types)

target_link_libraries(my_program PUBLIC value_types::value_types)
```

## Contributing Guidelines

- **Build Systems Synchronization**: Both CMake and Bazel are used as primary
  build systems. Whenever adding new targets or files, they must be manually
  added to both `CMakeLists.txt` and `BUILD.bazel`.

- **Code Formatting**: Code is formatted using `clang-format` (v17). This is
  enforced by GitHub actions and `pre-commit` hooks.

- **Testing**: All changes must have test coverage. Both CMake and Bazel builds
  and tests must pass after any change.
