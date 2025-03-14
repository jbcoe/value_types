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
      NAME <name>
      FILES <source_files...>
      LINK_LIBRARIES <libraries...>
  )
   -- Generates object library targets with default build settings.

  ``NAME``
    The ``NAME`` option is required to provide the name for the object library.

  ``FILES``
    The ``FILES`` option is required to specify the source files to be compiled.

  ``LINK_LIBRARIES``
    The ``LINK_LIBRARIES`` option is required to specify the libraries to link against.

#]=======================================================================]
function(xyz_add_object_library)
    set(options)
    set(oneValueArgs NAME)
    set(multiValueArgs FILES LINK_LIBRARIES)

    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_NAME)
        message(FATAL_ERROR "xyz_add_object_library: NAME parameter is required")
    endif()

    if(NOT ARG_FILES)
        message(FATAL_ERROR "xyz_add_object_library: FILES parameter is required")
    endif()

    if(NOT ARG_LINK_LIBRARIES)
        message(FATAL_ERROR "xyz_add_object_library: LINK_LIBRARIES parameter is required")
    endif()

    # Create the object library
    add_library(${ARG_NAME} OBJECT)

    # Add the source files
    target_sources(${ARG_NAME}
        PRIVATE
            ${ARG_FILES}
    )

    # Link against the base library
    target_link_libraries(${ARG_NAME}
        PRIVATE
            ${ARG_LINK_LIBRARIES}
    )
endfunction()
