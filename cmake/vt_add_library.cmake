include_guard(GLOBAL)


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
endfunction()
