# Copilot Instructions for value_types

## Repository Overview

This repository contains C++ header-only library implementations of two class templates: `indirect<T>` and `polymorphic<T>`. These are designed for composite class design to provide value semantics for types that require indirect storage or polymorphic behavior.

### Purpose
- `indirect<T>`: Owns an object of class `T` with value semantics (useful for PIMPL idiom, incomplete types)
- `polymorphic<T>`: Owns an object of class `T` or a class derived from `T` with value semantics (useful for open-set polymorphism)

### Standardization Status
`std::indirect` and `std::polymorphic` have been **accepted** into the C++ draft standard and should be available in C++26. The final proposal [P3019r14](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3019r14.pdf) was accepted in Plenary in Hagenburg 2025, following the initial acceptance of [P3019r11](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3019r11.pdf) in Wrocław 2024.

This repository provides a reference implementation that influenced the standard library design. The implementation documented in `DRAFT.md` follows strict standardization requirements and serves as a basis for the standardized versions.

#### Standard Library Documentation
- [`std::indirect`](https://en.cppreference.com/w/cpp/memory/indirect) - cppreference documentation
- [`std::polymorphic`](https://en.cppreference.com/w/cpp/memory/polymorphic) - cppreference documentation

## Development Environment

### Development Container
This repository includes a `.devcontainer` configuration that can and should be used for development work. The Docker container it defines sets up the build environment for both CMake and Bazel, ensuring consistent development across different systems.

### Language and Standards
- **Primary Language**: C++
- **Minimum Standard**: C++14
- **Supported Standards**: C++14, C++17, C++20, C++23
- **Style**: Follow the existing codebase style, which uses clang-format configuration

### Build Systems
This project supports **dual build systems** - both must be maintained:

1. **CMake** (Primary)
   ```bash
   cmake --preset Debug
   cmake --build --preset Debug
   ctest --preset Debug
   ```

2. **Bazel** (Secondary)
   ```bash
   bazel build //...
   bazel test //...
   ```

### Testing Framework
- **Primary**: Google Test (gtest/gmock)
- **Coverage**: Comprehensive test suite with 400+ tests
- **Pattern**: Each feature should have corresponding tests
- **Naming**: Use descriptive test names following existing patterns

## Code Organization

### Core Files
- `indirect.h` / `indirect.cc` - Main indirect implementation
- `polymorphic.h` / `polymorphic.cc` - Main polymorphic implementation
- `indirect_cxx14.h` / `indirect_cxx14.cc` - C++14 compatible version
- `polymorphic_cxx14.h` / `polymorphic_cxx14.cc` - C++14 compatible version
- `polymorphic_no_vtable.h` / `polymorphic_no_vtable.cc` - Alternative implementation

### Test Files
- `indirect_test.cc` - Tests for all indirect implementations
- `polymorphic_test.cc` - Tests for all polymorphic implementations

### Support Files
- `feature_check.h` - Feature detection macros
- `test_helpers.h` - Common test utilities
- `tagged_allocator.h` / `tracking_allocator.h` - Custom allocators for testing

## Coding Conventions

### Header Guards and Includes
- Use traditional `#ifndef`/`#define` header guards following existing patterns (e.g., `#ifndef XYZ_INDIRECT_H`)
- Include system headers first, then project headers
- Use forward declarations when possible

### Naming Conventions
- **Classes/Types**: `snake_case` (e.g., `indirect`, `polymorphic`)
- **Functions/Methods**: `snake_case` (e.g., `has_value`, `value_or`)
- **Template Parameters**: `PascalCase` (e.g., `T`, `Allocator`)
- **Namespaces**: Use `xyz` namespace for library code

### Code Style
- Follow the `.clang-format` configuration
- Use `const` and `constexpr` where appropriate
- Prefer RAII and value semantics
- Use standard library features when available
- Write exception-safe code

### Template and Generic Programming
- Use SFINAE and concepts for template constraints
- Provide clear error messages for template instantiation failures
- Support custom allocators following standard library patterns
- Use tag dispatching for constructor overload resolution

## Standard Library Compatibility

### Allocator Support
- Both `indirect` and `polymorphic` support custom allocators
- Follow standard allocator-aware container patterns
- Use `std::allocator_traits` for allocator operations
- Support both traditional and PMR allocators

### Special Member Functions
- Follow the Rule of Five/Zero
- Provide proper `noexcept` specifications
- Support move semantics with proper exception safety
- Implement copy/move constructors and assignment operators

### Value Semantics
- Maintain deep copy semantics for owned objects
- Ensure const-correctness throughout
- Support comparison operators (`==`, `!=`, `<`, etc.)

## Testing Patterns

### Test Structure
```cpp
TEST(ClassNameTest, DescriptiveFunctionality) {
    // Arrange
    // Act
    // Assert
}
```

### Common Test Categories
- **Construction**: Default, copy, move, in-place, allocator-extended
- **Assignment**: Copy, move, converting, self-assignment
- **Observers**: `has_value()`, `operator*`, `operator->`
- **Modifiers**: `swap()`, `reset()`, assignment operators
- **Comparison**: Equality, ordering, comparison with T
- **Exception Safety**: Strong exception guarantee verification
- **Allocator Integration**: Custom allocator behavior

### Memory Management Testing
- Use `tracking_allocator` to verify allocation patterns
- Test allocator propagation in copy/move operations
- Verify no memory leaks with RAII patterns
- Test exception safety with throwing allocators

## Implementation Guidelines

### Error Handling
- Use exceptions for error conditions (following standard library conventions)
- Provide strong exception safety guarantee
- Use `assert` for precondition checks in debug builds
- Avoid throwing from destructors

### Performance Considerations
- Minimize dynamic allocations
- Use move semantics to avoid unnecessary copies
- Provide `noexcept` operations where possible
- Consider cache locality in data layout

### Platform Support
- Write portable C++ code
- Use standard library features over platform-specific code
- Test on multiple compilers (GCC, Clang, MSVC)
- Support both 32-bit and 64-bit platforms

## Documentation

### Code Documentation
- Use clear, descriptive names for functions and variables
- Add comments for complex algorithms or non-obvious behavior
- Document template parameters and requirements
- Include usage examples in headers

### API Documentation
- Follow standard library documentation patterns
- Document preconditions, postconditions, and exception guarantees
- Provide complexity guarantees where relevant
- Include rationale for design decisions

## Common Development Tasks

### Adding New Features
1. Update both main and C++14 versions if needed
2. Add corresponding tests in the appropriate test file
3. Update both CMake and Bazel build files
4. Update documentation if API changes
5. Run full test suite and check formatting

### Debugging Issues
- Use the compile checks in `compile_checks/` directory
- Check allocator tracking tests for memory issues
- Verify exception safety with throwing operations
- Use static_assert for compile-time validation

## Review and Quality Assurance

### Before Submitting Changes
- Run both CMake and Bazel builds
- Execute full test suite (should be 400+ tests passing)
- Check code formatting with clang-format
- Run clang-tidy for static analysis
- Verify no new compiler warnings
- Update documentation if needed

### Continuous Integration
- GitHub Actions run multiple build configurations
- Tests run on different platforms and compilers
- Code coverage is tracked and should not decrease
- Pre-commit hooks enforce formatting and basic checks
