cmake_minimum_required(VERSION 3.22.4 FATAL_ERROR)
cmake_policy(SET CMP0127 NEW)

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
cmake_dependent_option(ENABLE_INCLUDE_NATVIS "Enable inclusion of a natvis file for debugging" ON "\"${CMAKE_CXX_COMPILER_ID}\" STREQUAL \"MSVC\"" OFF)
option(ENABLE_SANITIZERS "Enable Address Sanitizer and Undefined Behaviour Sanitizer if available" OFF)

add_library(value_types INTERFACE)
target_include_directories(value_types
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_sources(value_types
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/indirect.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/polymorphic.h>
        # Only include natvis files in Visual Studio
    #    $<BUILD_INTERFACE:$<$<CXX_COMPILER_ID:MSVC>:${CMAKE_CURRENT_SOURCE_DIR}/indirect_value.natvis>>
    #    $<INSTALL_INTERFACE:$<$<BOOL:${ENABLE_INCLUDE_NATVIS}>:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}/indirect_value.natvis>>
)

target_compile_features(value_types
    INTERFACE
        cxx_std_23
)

add_library(indirect INTERFACE)
target_include_directories(indirect
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_sources(indirect
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/indirect.h>
)

target_compile_features(indirect
    INTERFACE
        cxx_std_23
)

add_library(polymorphic INTERFACE)
target_include_directories(polymorphic
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_sources(polymorphic
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/polymorphic.h>
)

target_compile_features(polymorphic
    INTERFACE
        cxx_std_23
)

add_library(value_types::value_types ALIAS value_types)

if (${CPP_VALUE_TYPES_IS_NOT_SUBPROJECT})

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
        FetchContent_MakeAvailable(benchmark)

        add_library(common_compiler_settings INTERFACE)

        target_compile_options(common_compiler_settings
            INTERFACE
                $<$<CXX_COMPILER_ID:MSVC>:/EHsc>
                $<$<CXX_COMPILER_ID:MSVC>:/W4>
                $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
                $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:Clang>>:-Werror;-Wall;-Wno-self-assign-overloaded;-Wno-unknown-warning-option;-Wno-unused-but-set-variable>
        )

        set_target_properties(common_compiler_settings PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS NO
        )
        
        add_executable(indirect_test "")
        target_sources(indirect_test
            PRIVATE
                indirect_test.cc
        )

        target_link_libraries(indirect_test
            PRIVATE
                indirect
                GTest::gtest_main
                common_compiler_settings
        )

        add_executable(polymorphic_test "")
        target_sources(polymorphic_test
            PRIVATE
                polymorphic_test.cc
        )

        target_link_libraries(polymorphic_test
            PRIVATE
                polymorphic
                GTest::gtest_main
                common_compiler_settings
        )

        add_executable(value_types_benchmark "")
        target_sources(value_types_benchmark
            PRIVATE
                indirect_benchmark.cc
                polymorphic_benchmark.cc
        )
        target_link_libraries(value_types_benchmark
        PRIVATE
            polymorphic
            indirect
            benchmark::benchmark_main
            common_compiler_settings
        )

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
        add_test(
            NAME indirect_test
            COMMAND indirect_test
            WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
        add_test(
            NAME polymorphic_test
            COMMAND polymorphic_test
            WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

        list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/contrib)
        include(GoogleTest)
        gtest_discover_tests(indirect_test)
        gtest_discover_tests(polymorphic_test)

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
            add_coverage(indirect_test)
            add_coverage(polymorphic_test)
            list(APPEND LCOV_REMOVE_PATTERNS "'/usr/*'")
            coverage_evaluate()
        endif()
    endif(${BUILD_TESTING})

    install(
        FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/indirect.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/polymorphic.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/value_types.natvis"
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
