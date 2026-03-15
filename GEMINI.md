# Project Overview

**value_types** is a C++ header-only library that provides two class templates:
`xyz::indirect` and `xyz::polymorphic`. These templates are designed for member
data in composite types to act as value types, managing object ownership and
significantly reducing the boilerplate required for special member functions.

Both `std::indirect` and `std::polymorphic` have been accepted into the C++
draft standard for C++26. This repository provides implementations for C++20 and
also C++14 (`indirect_cxx14.h` and `polymorphic_cxx14.h`).

## Main Technologies

- **Language**: C++ (C++14 and C++20)
- **Build Systems**: CMake and Bazel
- **Tools**: `uv` (for Python virtual environments), `pre-commit`,
  `clang-format` (v17), GitHub Actions for CI.

## Building and Testing

### Using CMake

To build and test using CMake, you can utilize the provided presets:

```bash
mkdir build                     # Make a build directory
cmake --list-presets            # View the available presets
cmake --preset <preset>         # Generate build system specified in build directory with cmake
cmake --build --preset <preset> # Build the underlying build system via CMake
ctest --preset <preset>         # Run the tests
```

### Using Bazel

To build and test using Bazel:

```bash
bazel build //...       # Build the project
bazel test //...        # Run the tests
```

## Development Conventions

- **Ambiguity**: If any instructions are unclear or ambiguous, then you must ask
  the user to clarify before executing any actions.

- **Build Systems Synchronization**: Both CMake and Bazel are used as primary
  build systems. Whenever adding new targets or files, they must be manually
  added to both `CMakeLists.txt` and `BUILD.bazel`.

- **Git Hooks**: The repository uses `pre-commit` managed via the `uv` tool to
  run git hooks prior to commits. Install it using:

  ```bash
  uv run pre-commit install
  ```

- **Code Formatting**: Code is formatted using `clang-format` (v17). This is
  enforced by GitHub actions and `pre-commit` hooks.

- **Testing**: All changes must have test coverage, ensured by GitHub actions.
  Ensure that any code changes are accompanied by corresponding tests. Tests and
  pre-commit checks must pass after any change.
