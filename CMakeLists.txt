cmake_minimum_required(VERSION 3.22.4 FATAL_ERROR)
cmake_policy(SET CMP0127 NEW)
cmake_policy(SET CMP0135 NEW)

if(NOT DEFINED PROJECT_NAME)
    set(XYZ_VALUE_TYPES_IS_NOT_SUBPROJECT ON)
endif()

set(XYZ_VALUE_TYPES_VERSION 0.0.1)

project(value_types LANGUAGES CXX VERSION ${XYZ_VALUE_TYPES_VERSION})

include(CTest)
include(FetchContent)
include(GNUInstallDirs)
include(CMakeDependentOption)
include(CMakePackageConfigHelpers)

option(ENABLE_SANITIZERS "Enable Address Sanitizer and Undefined Behaviour Sanitizer if available" OFF)

# Include necessary submodules
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# Custom CMake Includes
include(coverage)
include(xyz_add_library)
include(xyz_add_test)

xyz_add_library(
    NAME xyz_value_types
    ALIAS xyz_value_types::value_types
)
target_sources(xyz_value_types
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/indirect.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/polymorphic.h>
)

xyz_add_library(
    NAME indirect
    ALIAS xyz_value_types::indirect
    DEFINITIONS XYZ_HAS_EXTENSION_MAKE_INDIRECT
)
target_sources(indirect
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/indirect.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/indirect.cc>
)

xyz_add_library(
    NAME indirect_cxx14
    ALIAS xyz_value_types::indirect_cxx14
    DEFINITIONS XYZ_INDIRECT_CXX_14
    VERSION 14
)
target_sources(indirect_cxx14
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/indirect_cxx14.h>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/indirect_cxx14.cc>
)

xyz_add_library(
    NAME indirect_cxx17
    ALIAS xyz_value_types::indirect_cxx17
    DEFINITIONS XYZ_INDIRECT_CXX_14
    VERSION 17
)
target_sources(indirect_cxx17
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/indirect_cxx14.h>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/indirect_cxx14.cc>
)

xyz_add_library(
    NAME polymorphic
    ALIAS xyz_value_types::polymorphic
    DEFINITIONS XYZ_HAS_EXTENSION_MAKE_POLYMORPHIC
)
target_sources(polymorphic
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/polymorphic.h>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/polymorphic.cc>
)

xyz_add_library(
    NAME polymorphic_cxx14
    ALIAS xyz_value_types::polymorphic_cxx14
    DEFINITIONS XYZ_POLYMORPHIC_CXX_14
    VERSION 14
)
target_sources(polymorphic_cxx14
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/polymorphic_cxx14.h>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/polymorphic_cxx14.cc>
)

xyz_add_library(
    NAME polymorphic_cxx17
    ALIAS xyz_value_types::polymorphic_cxx17
    DEFINITIONS XYZ_POLYMORPHIC_CXX_14
    VERSION 17
)
target_sources(polymorphic_cxx17
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/polymorphic_cxx14.h>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/polymorphic_cxx14.cc>
)

if (${XYZ_VALUE_TYPES_IS_NOT_SUBPROJECT})

    add_subdirectory(benchmarks)
    add_subdirectory(compile_checks)
    add_subdirectory(exploration)

    if (${BUILD_TESTING})
        FetchContent_Declare(
          googletest
          GIT_REPOSITORY https://github.com/google/googletest.git
          GIT_TAG        v1.15.2
        )
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)

        FetchContent_Declare(
            benchmark
            GIT_REPOSITORY https://github.com/google/benchmark.git
            GIT_TAG        v1.8.3
        )
        set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(benchmark)

        enable_testing()

        xyz_add_test(
            NAME indirect_test
            LINK_LIBRARIES indirect
            FILES indirect_test.cc
        )

        xyz_add_test(
            NAME indirect_cxx14_test
            LINK_LIBRARIES indirect_cxx14
            FILES indirect_test.cc
            VERSION 14
        )

        xyz_add_test(
            NAME indirect_cxx17_test
            LINK_LIBRARIES indirect_cxx17
            FILES indirect_test.cc
            VERSION 17
        )

        xyz_add_test(
            NAME polymorphic_test
            LINK_LIBRARIES polymorphic
            FILES polymorphic_test.cc
        )

        xyz_add_test(
            NAME polymorphic_cxx14_test
            LINK_LIBRARIES polymorphic_cxx14
            FILES polymorphic_test.cc
            VERSION 14
        )

        xyz_add_test(
            NAME polymorphic_cxx17_test
            LINK_LIBRARIES polymorphic_cxx17
            FILES polymorphic_test.cc
            VERSION 17
        )

        if (ENABLE_CODE_COVERAGE)
            enable_code_coverage()
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
            "${CMAKE_INSTALL_DATAROOTDIR}/licenses/xyz_value_types"
    )

    install(
        TARGETS xyz_value_types
        EXPORT xyz_value_types-export-set
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(
        EXPORT xyz_value_types-export-set
        NAMESPACE xyz_value_types::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/xyz_value_types"
    )

    configure_package_config_file(
        ${CMAKE_CURRENT_LIST_DIR}/xyz_value_types-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/xyz_value_types-config.cmake
        INSTALL_DESTINATION
            "${CMAKE_INSTALL_LIBDIR}/cmake/xyz_value_types"
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/xyz_value_types-version.cmake
        VERSION ${XYZ_VALUE_TYPES_VERSION}
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT
    )

    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/xyz_value_types-config.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/xyz_value_types-version.cmake
        DESTINATION
            "${CMAKE_INSTALL_LIBDIR}/cmake/xyz_value_types"
    )
endif()
