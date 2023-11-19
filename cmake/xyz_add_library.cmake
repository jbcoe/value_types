include_guard(GLOBAL)

#[=======================================================================[.rst:
xyz_add_library
------------------

Overview
^^^^^^^^

Project wrapper around add library which groups commonly associates patterns
and allows configuration for common optional settings

.. code-block:: cmake

  xyz_add_library(
      [NAME <name>]
      [ALIAS <alias>]
      [VERSION <version>]
      [DEFINITIONS <definitions...>]
  )
   -- Generates library targets with default build directories and install options.

  ``NAME``
    The ``NAME`` option is required to provide the internal name for the library.

  ``ALIAS``
    The ``ALIAS`` option is required to provide the external name for the library.

  ``VERSION``
    The ``VERSION`` option specifies the supported C++ version.

  ``DEFINITIONS``
    The ``DEFINITIONS`` option provides a list of compile definitions.

#]=======================================================================]
function(xyz_add_library)
    set(options)
    set(oneValueArgs NAME ALIAS VERSION)
    set(multiValueArgs DEFINITIONS)
    cmake_parse_arguments(XYZ "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if (NOT XYZ_NAME)
        message(FATAL_ERROR "NAME parameter must be supplied")
    endif()
    if (NOT XYZ_ALIAS)
        message(FATAL_ERROR "ALIAS parameter must be supplied")
    endif()
    if (NOT XYZ_VERSION)
        set(XYZ_CXX_STANDARD cxx_std_20)
    else()
        set(VALID_TARGET_VERSIONS 11 14 17 20 23)
        list(FIND VALID_TARGET_VERSIONS ${XYZ_VERSION} index)
        if(index EQUAL -1)
            message(FATAL_ERROR "TYPE must be one of <${VALID_TARGET_VERSIONS}>")
        endif()
        set(XYZ_CXX_STANDARD cxx_std_${XYZ_VERSION})
    endif()

    add_library(${XYZ_NAME} INTERFACE)
    add_library(${XYZ_ALIAS} ALIAS ${XYZ_NAME})
    target_include_directories(${XYZ_NAME}
        INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
    target_compile_features(${XYZ_NAME}
        INTERFACE
            ${XYZ_CXX_STANDARD}
    )
    if (XYZ_DEFINITIONS)
        target_compile_definitions(${XYZ_NAME}
            INTERFACE
                ${XYZ_DEFINITIONS}
        )
    endif (XYZ_DEFINITIONS)

endfunction()
