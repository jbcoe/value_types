include_guard(GLOBAL)

#[=======================================================================[.rst:
vt_add_test
------------------

Overview
^^^^^^^^

Project wrapper around add execcuable for test excuables which groups commonly
associates patterns and allows configuration for common optional settings

.. code-block:: cmake

  vt_add_test(
      [NAME <name>]
  )
   -- Generates test executable targets with default build directories and settings.

  ``NAME``
    The ``NAME`` option is required to provide the internal name for the library.

#]=======================================================================]
function(vt_add_test)
    set(options MANUAL)
    set(oneValueArgs NAME MANUAL)
    set(multiValueArgs LINK_LIBRARIES FILES)
    cmake_parse_arguments(VALUE_TYPES "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if (NOT VALUE_TYPES_NAME)
        message(FATAL_ERROR "NAME parameter must be supplied")
    endif()

    if (NOT TARGET common_compiler_settings)
        add_library(common_compiler_settings INTERFACE)
        target_compile_options(common_compiler_settings
            INTERFACE
                $<$<CXX_COMPILER_ID:MSVC>:/EHsc>
                $<$<CXX_COMPILER_ID:MSVC>:/W4>
                $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
                $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:Clang>>:-Werror;-Wall;-Wno-self-assign-overloaded;-Wno-delete-non-abstract-non-virtual-dtor;-Wno-unknown-warning-option;-Wno-self-move>
        )

    endif (NOT TARGET common_compiler_settings)

    include(sanitizers)

    add_executable(${VALUE_TYPES_NAME} "")
    target_sources(${VALUE_TYPES_NAME}
       PRIVATE
            ${VALUE_TYPES_FILES}
    )
    target_link_libraries(${VALUE_TYPES_NAME}
        PRIVATE
            ${VALUE_LINK_LIBRARIES}
            GTest::gtest_main
            common_compiler_settings
            $<$<BOOL:${COMPILER_SUPPORTS_ASAN}>:asan>
            $<$<BOOL:${COMPILER_SUPPORTS_USAN}>:ubsan>
    )

    set_target_properties(${VALUE_TYPES_NAME} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )

    if(${VALUE_TYPES_MANUAL})
        message(STATUS "Manual test: ${VALUE_TYPES_NAME}")
    else()
        add_test(
            NAME ${VALUE_TYPES_NAME}
            COMMAND ${VALUE_TYPES_NAME}
            WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        )
    endif()

    include(GoogleTest)
    gtest_discover_tests(${VALUE_TYPES_NAME})

    if (ENABLE_CODE_COVERAGE)
        add_coverage(${VALUE_TYPES_NAME})
    endif()

endfunction()
