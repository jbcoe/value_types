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
      [VERSION <version>]
      [FILES <files...>]
      [LINK_LIBRARIES <libraries...>]
  )
   -- Generates test executable targets with default build directories and settings.

  ``NAME``
    The ``NAME`` option is required to provide the internal name for the library.

  ``VERSION``
    The ``VERSION`` option specifies the supported C++ version.

  ``FILES``
    The ``FILES`` parameter support a list of input files for the executable target.

  ``LINK_LIBRARIES``
    The ``LINK_LIBRARIES`` parameter support as list of libraries which the
    target depends upon.

#]=======================================================================]
function(vt_add_test)
    set(options)
    set(oneValueArgs NAME VERSION)
    set(multiValueArgs LINK_LIBRARIES FILES)
    cmake_parse_arguments(VALUE_TYPES "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if (NOT VALUE_TYPES_NAME)
        message(FATAL_ERROR "NAME parameter must be supplied")
    endif()
    if (NOT VALUE_TYPES_VERSION)
        set(VALUE_TYPES_VERSION 20)
    else()
        set(VALID_TARGET_VERSIONS 11 14 17 20 23)
        list(FIND VALID_TARGET_VERSIONS ${VALUE_TYPES_VERSION} index)
        if(index EQUAL -1)
            message(FATAL_ERROR "TYPE must be one of <${VALID_TARGET_TYPES}>")
        endif()
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
            ${VALUE_TYPES_LINK_LIBRARIES}
            GTest::gtest_main
            common_compiler_settings
            $<$<BOOL:${COMPILER_SUPPORTS_ASAN}>:asan>
            $<$<BOOL:${COMPILER_SUPPORTS_USAN}>:ubsan>
    )

    set_target_properties(${VALUE_TYPES_NAME} PROPERTIES
        CXX_STANDARD ${VALUE_TYPES_VERSION}
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )

    add_test(
        NAME ${VALUE_TYPES_NAME}
        COMMAND ${VALUE_TYPES_NAME}
        WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )

    include(GoogleTest)
    gtest_discover_tests(${VALUE_TYPES_NAME})

    if (ENABLE_CODE_COVERAGE)
        add_coverage(${VALUE_TYPES_NAME})
    endif()

endfunction()
