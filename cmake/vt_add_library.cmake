include_guard(GLOBAL)

#[=======================================================================[.rst:
vt_add_library
------------------

Overview
^^^^^^^^

Project wrapper around add library which groups commonly associates patterns
and allows configuration for common optional settings

.. code-block:: cmake

  vt_add_library(
      [NAME <name>]
      [ALIAS <alias>]
      [DEFINITIONS <definitions...>]
  )
   -- Generates library targets with default build directories and install options.

  ``NAME``
    The ``NAME`` option is required to provide the internal name for the library.

  ``ALIAS``
    The ``ALIAS`` option is required to provide the external name for the library.

  ``DEFINITIONS``
    The ``DEFINITIONS`` option provides a list of compile definitions.

#]=======================================================================]
function(vt_add_library)
    set(options)
    set(oneValueArgs NAME ALIAS)
    set(multiValueArgs)
    cmake_parse_arguments(VALUE_TYPES "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if (NOT VALUE_TYPES_NAME)
        message(FATAL_ERROR "NAME parameter must be supplied")
    endif()
    if (NOT VALUE_TYPES_ALIAS)
        message(FATAL_ERROR "ALIAS parameter must be supplied")
    endif()

    add_library(${VALUE_TYPES_NAME} INTERFACE)
    add_library(${VALUE_TYPES_ALIAS} ALIAS ${VALUE_TYPES_NAME})
    target_include_directories(${VALUE_TYPES_NAME}
        INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
    target_compile_features(${VALUE_TYPES_NAME}
        INTERFACE
            cxx_std_20
    )
    if (VALUE_DEFINITIONS)
        target_compile_definitions(polymorphic_sbo
            INTERFACE
                ${VALUE_DEFINITIONS}
        )
    endif (VALUE_DEFINITIONS)

endfunction()
