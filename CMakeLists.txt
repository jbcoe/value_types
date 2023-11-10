cmake_minimum_required(VERSION 3.22.4 FATAL_ERROR)
cmake_policy(SET CMP0127 NEW)
cmake_policy(SET CMP0135 NEW)

if(NOT DEFINED PROJECT_NAME)
    set(CPP_VALUE_TYPES_IS_NOT_SUBPROJECT ON)
endif()

set(CPP_VALUE_TYPES_VERSION 0.0.1)

project(value_types LANGUAGES CXX VERSION ${CPP_VALUE_TYPES_VERSION})

include(CTest)
include(FetchContent)
include(GNUInstallDirs)
include(CMakeDependentOption)
include(CMakePackageConfigHelpers)

# Ensure code coverage is switched off for Mac OS until the code coverage library addresses the AppleClang issue
cmake_dependent_option(ENABLE_CODE_COVERAGE "Enable code coverage" ON "\"${CMAKE_CXX_COMPILER_ID}\" STREQUAL \"Clang\" OR \"${CMAKE_CXX_COMPILER_ID}\" STREQUAL \"GNU\"" OFF)
option(ENABLE_SANITIZERS "Enable Address Sanitizer and Undefined Behaviour Sanitizer if available" OFF)

# Include necessary submodules
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# Custom CMake Includes
include(vt_add_library)
include(vt_add_test)

vt_add_library(
    NAME value_types
    ALIAS value_types::value_types
)
target_sources(value_types
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/indirect.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/polymorphic.h>
)

vt_add_library(
    NAME indirect
    ALIAS value_types::indirect
)
target_sources(indirect
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/indirect.h>
)

vt_add_library(
    NAME polymorphic
    ALIAS value_types::polymorphic
)
target_sources(polymorphic
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/polymorphic.h>
)

vt_add_library(
    NAME polymorphic_inline_vtable
    ALIAS value_types::polymorphic_inline_vtable
)
target_sources(polymorphic_inline_vtable
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/polymorphic.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/experimental/polymorphic_inline_vtable.h>
)
target_compile_definitions(polymorphic_inline_vtable
    INTERFACE
        XYZ_POLYMORPHIC_USES_EXPERIMENTAL_INLINE_VTABLE
)

vt_add_library(
    NAME polymorphic_sbo
    ALIAS value_types::polymorphic_sbo
)
target_sources(polymorphic_sbo
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/polymorphic.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/experimental/polymorphic_sbo.h>
)
target_compile_definitions(polymorphic_sbo
    INTERFACE
        XYZ_POLYMORPHIC_USES_EXPERIMENTAL_SMALL_BUFFER_OPTIMIZATION
)

if (${CPP_VALUE_TYPES_IS_NOT_SUBPROJECT})

    add_subdirectory(benchmarks)

    if(!MSVC)
        # There are issues with constexpr/consteval in MSVC 2019 and MSVC 2022.
        add_subdirectory(compile_checks)
    endif(!MSVC)

    if (${BUILD_TESTING})
        FetchContent_Declare(
          googletest
          URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
        )
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)

        FetchContent_Declare(
            benchmark
            URL https://github.com/google/benchmark/archive/v1.8.3.zip
        )
        set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(benchmark)

        if (ENABLE_SANITIZERS)
            set(SANITIZER_FLAGS_ASAN "-fsanitize=address -fno-omit-frame-pointer")
            set(SANITIZER_FLAGS_UBSAN "-fsanitize=undefined")

            include(CheckCXXCompilerFlag)
            check_cxx_compiler_flag("${SANITIZER_FLAGS_ASAN}" COMPILER_SUPPORTS_ASAN)
            check_cxx_compiler_flag("${SANITIZER_FLAGS_UBSAN}" COMPILER_SUPPORTS_UBSAN)

            if (COMPILER_SUPPORTS_ASAN)
                add_library(asan INTERFACE IMPORTED)
                set_target_properties(asan PROPERTIES
                    INTERFACE_COMPILE_OPTIONS "${SANITIZER_FLAGS_ASAN}"
                    INTERFACE_LINK_OPTIONS "${SANITIZER_FLAGS_ASAN}"
                )
                target_link_libraries(indirect_test
                    PRIVATE
                        asan
                )
                target_link_libraries(polymorphic_test
                    PRIVATE
                        asan
                )
            endif(COMPILER_SUPPORTS_ASAN)

            if (COMPILER_SUPPORTS_UBSAN)
                add_library(ubsan INTERFACE IMPORTED)
                set_target_properties(ubsan PROPERTIES
                    INTERFACE_COMPILE_OPTIONS "${SANITIZER_FLAGS_UBSAN}"
                    INTERFACE_LINK_OPTIONS "${SANITIZER_FLAGS_UBSAN}"
                )
                target_link_libraries(indirect_test
                    PRIVATE
                        ubsan
                )
                target_link_libraries(polymorphic_test
                    PRIVATE
                        ubsan
                )
            endif(COMPILER_SUPPORTS_UBSAN)
        endif(ENABLE_SANITIZERS)

        enable_testing()

        if (ENABLE_CODE_COVERAGE)
            FetchContent_Declare(
                codecoverage
                GIT_REPOSITORY https://github.com/RWTH-HPC/CMake-codecov.git
            )

            FetchContent_GetProperties(codecoverage)
            if(NOT codecoverage_POPULATED)
                FetchContent_Populate(codecoverage)
                list(APPEND CMAKE_MODULE_PATH ${codecoverage_SOURCE_DIR}/cmake)
            endif()

            set(ENABLE_COVERAGE ON CACHE BOOL "Enable coverage build." FORCE)
            find_package(codecov)
            list(APPEND LCOV_REMOVE_PATTERNS "'/usr/*'")
        endif()

        vt_add_test(
            NAME indirect_test
            LINK_LIBRARIES indirect
            FILES indirect_test.cc
        )

        vt_add_test(
            NAME polymorphic_test
            LINK_LIBRARIES polymorphic
            FILES polymorphic_test.cc
        )

        vt_add_test(
            NAME polymorphic_inline_vtable_test
            LINK_LIBRARIES polymorphic_inline_vtable
            FILES polymorphic_test.cc
        )

        vt_add_test(
            NAME polymorphic_sbo_test
            LINK_LIBRARIES polymorphic_sbo
            FILES polymorphic_test.cc
        )
        target_compile_options(polymorphic_sbo_test
            PRIVATE
                $<$<CXX_COMPILER_ID:Clang>:-ftemplate-backtrace-limit=0>
        )

        if (ENABLE_CODE_COVERAGE)
            coverage_evaluate()
        endif()

    endif(${BUILD_TESTING})

    install(
        FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/indirect.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/polymorphic.h"
        DESTINATION
            ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(
        FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt"
        DESTINATION
            "${CMAKE_INSTALL_DATAROOTDIR}/licenses/value_types"
    )

    install(
        TARGETS value_types
        EXPORT value_types-export-set
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(
        EXPORT value_types-export-set
        NAMESPACE value_types::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/value_types"
    )

    configure_package_config_file(
        ${CMAKE_CURRENT_LIST_DIR}/value_types-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/value_types-config.cmake
        INSTALL_DESTINATION
            "${CMAKE_INSTALL_LIBDIR}/cmake/value_types"
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/value_types-version.cmake
        VERSION ${CPP_VALUE_TYPES_VERSION}
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT
    )

    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/value_types-config.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/value_types-version.cmake
        DESTINATION
            "${CMAKE_INSTALL_LIBDIR}/cmake/value_types"
    )
endif()
