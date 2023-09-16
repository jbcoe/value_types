cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
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

add_library(value_types::value_types ALIAS value_types)

if (${CPP_VALUE_TYPES_IS_NOT_SUBPROJECT})

    if (${BUILD_TESTING})
        FetchContent_Declare(
            catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.4.0
        )

        FetchContent_Declare(
          googletest
          URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
        )
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)

        # TODO(Avoid gloablly setting C++ standard) Force C++23 for Catchv2
        # Fix for: https://stackoverflow.com/questions/66227246/catch2-undefined-reference-to-catchstringmaker
        set(CMAKE_CXX_STANDARD 23)

        if(NOT catch2_POPULATED)
            FetchContent_Populate(catch2)
            add_subdirectory(${catch2_SOURCE_DIR} ${catch2_BINARY_DIR})
        endif()

        add_executable(value_types_test "")
        target_sources(value_types_test
            PRIVATE
                #pimpl.h
                #pimpl.cpp
                #pimpl_test.cpp
                indirect_test.cc
                polymorphic_test.cc
        )

        target_link_libraries(value_types_test
            PRIVATE
                value_types::value_types
                Catch2::Catch2WithMain
        )

        target_compile_options(value_types_test
            PRIVATE
                $<$<CXX_COMPILER_ID:MSVC>:/EHsc>
                $<$<CXX_COMPILER_ID:MSVC>:/W4>
                $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
                $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:Clang>>:-Werror;-Wall;-Wno-self-assign-overloaded;-Wno-unknown-warning-option>
        )

        set_target_properties(value_types_test PROPERTIES
            CXX_STANDARD 23
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS NO
        )

        add_executable(value_types_gtest "")
        target_sources(value_types_gtest
            PRIVATE
                indirect_gtest.cc
        )

        target_link_libraries(value_types_gtest
            PRIVATE
                value_types::value_types
                GTest::gtest_main
        )

        target_compile_options(value_types_gtest
            PRIVATE
                $<$<CXX_COMPILER_ID:MSVC>:/EHsc>
                $<$<CXX_COMPILER_ID:MSVC>:/W4>
                $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
                $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:Clang>>:-Werror;-Wall;-Wno-self-assign-overloaded;-Wno-unknown-warning-option>
        )

        set_target_properties(value_types_gtest PROPERTIES
            CXX_STANDARD 23
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS NO
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
                target_link_libraries(indirect_value_test
                    PRIVATE
                        asan
                )
                target_link_libraries(indirect_value_gtest
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
                target_link_libraries(value_types_test
                    PRIVATE
                        ubsan
                )
                target_link_libraries(value_types_gtest
                    PRIVATE
                        ubsan
                )
            endif(COMPILER_SUPPORTS_UBSAN)
        endif(ENABLE_SANITIZERS)

        enable_testing()
        add_test(
            NAME value_types_test
            COMMAND value_types_test
            WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
        add_test(
            NAME value_types_gtest
            COMMAND value_types_gtest
            WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

        list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/contrib)
        include(Catch)
        catch_discover_tests(value_types_test)
        include(GoogleTest)
        gtest_discover_tests(value_types_gtest)

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
            add_coverage(value_types_test)
            add_coverage(value_types_gtest)
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
