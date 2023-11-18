# Using pre-commit for git hooks

This repository uses the Python `pre-commit` library to manage git hook run as
part of the commit process.  Use the following steps to install a virtual
environment with pre-commmit set up, and then use precommit to install git hooks
it to your local repository.

```bash
cd <project root>
python3 -m venv .venv           # Create a Python virtual env
source ./.venv/bin/activate     # Activate the virtual env for bash by source.
pip install -r requirements.txt # Install latest requirements including pre-commit
pre-commit install              # Use pre-commit to install git hooks into the working repository.
```

# Building with CMake

To build the repository with CMake use the following steps
```bash
cd <project root>
mkdir build          # Make a build directory
cd build             # Switch into the build directory
cmake ../            # Generate build system specified in root with cmake
cmake --build ./     # Build the underlying build system via CMake
```

# Including in your own project

To use the value types code in your own CMake project then you can pull
the project in as a dependency via CMake's FetchContent module as follows:

```
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

# Contributing

We use GitHub actions to ensure that all changes have test coverage and that
source code is formatted with clang-format (v17).

We use CMake and Bazel. New targets are manually added to both.
