include_guard(GLOBAL)

#[=======================================================================[.rst:
xyz_add_object_library
------------------

Overview
^^^^^^^^
Project wrapper around add_library(OBJECT) which groups commonly associated patterns
and allows configuration for common optional settings.

.. code-block:: cmake

  xyz_add_object_library(
      [NAME <name>]
      [FILES <source_files...>]
      [LINK_LIBRARIES <libraries...>]
      [VERSION <version>]
      [DEFINITIONS <definitions...>]
  )
   -- Generates object library targets with default build settings.

  ``NAME``
    The ``NAME`` option is required to provide the name for the object library.

  ``FILES``
    The ``FILES`` option is required to specify the source files to be compiled.

  ``LINK_LIBRARIES``
    The ``LINK_LIBRARIES`` option is required to specify the libraries to link against.

  ``VERSION``
    The ``VERSION`` option specifies the supported C++ version.

  ``DEFINITIONS``
    The ``DEFINITIONS`` option provides a list of compile definitions.

#]=======================================================================]
function(xyz_add_object_library)
    set(options)
    set(oneValueArgs NAME VERSION)
    set(multiValueArgs FILES LINK_LIBRARIES DEFINITIONS)

    cmake_parse_arguments(XYZ "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT XYZ_NAME)
        message(FATAL_ERROR "NAME parameter must be supplied")
    endif()

    if(NOT XYZ_FILES)
        message(FATAL_ERROR "FILES parameter must be supplied")
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

    add_library(${XYZ_NAME} OBJECT)

    target_sources(${XYZ_NAME}
        PRIVATE
            ${XYZ_FILES}
    )

    target_compile_features(${XYZ_NAME}
        PRIVATE
            ${XYZ_CXX_STANDARD}
    )

    if(XYZ_LINK_LIBRARIES)
        target_link_libraries(${XYZ_NAME}
            PRIVATE
                ${XYZ_LINK_LIBRARIES}
        )
    endif()

    if (XYZ_DEFINITIONS)
        target_compile_definitions(${XYZ_NAME}
            PRIVATE
                ${XYZ_DEFINITIONS}
        )
    endif (XYZ_DEFINITIONS)

endfunction()
