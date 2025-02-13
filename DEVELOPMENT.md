# Developer Guide

## Using pre-commit for git hooks

This repository uses the Python `pre-commit` library to manage git hook run as
part of the commit process.  Use the following steps from the project root to
install a virtual environment with pre-commmit set up, and then use precommit to
install git hooks it to your local repository:

```bash
python3 -m venv .venv           # Create a Python virtual env
source ./.venv/bin/activate     # Activate the virtual env for bash by source.
pip install -r requirements.txt # Install latest requirements including pre-commit
pre-commit install              # Use pre-commit to install git hooks into the working repository.
```

## Building and testing

### Building with CMake

To build the repository with CMake use the following steps from the project root:

```bash
mkdir build                     # Make a build directory
cmake --list-presets            # View the available presets
cmake --preset <preset>         # Generate build system specified in build directory with cmake
cmake --build --preset <preset> # Build the underlying build system via CMake
ctest --preset <preset>         # Run the tests
```

To install CMake see: https://cmake.org/download/.

### Building with Bazel

To build the repository with Bazel use the following steps from the project root:

```bash
bazel build //...       # Build the project
bazel test //...        # Run the tests
```

To install Bazel see https://bazel.build/install.

## Including value_types to your own project

To use the value types code in your own CMake project then you can pull
the project in as a dependency via CMake's FetchContent module as follows:

```txt
FetchContent_Declare(
    value_types
    GIT_REPOSITORY https://github.com/jbcoe/value_types
)
FetchContent_MakeAvailable(value_types)

add_executable(my_program)
target_link_libraries(my_program
    PUBLIC
        value_types::value_types
)
```

## Contributing

We use GitHub actions to ensure that all changes have test coverage and that
source code is formatted with clang-format (v17).

We use CMake and Bazel. New targets are manually added to both.
