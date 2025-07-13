include_guard(GLOBAL)

include(CheckCXXCompilerFlag)

option(ENABLE_CODE_COVERAGE "Enable code coverage" OFF)

set(COVERAGE_SUPPORTED_FLAGS
    # gcc 8 onwards
    "-fprofile-arcs -fprofile-abs-path -ftest-coverage"

    # gcc and clang
    "-fprofile-arcs -ftest-coverage"

    # gcc and clang fallback
    "--coverage"
)

#[=======================================================================[.rst:
virtualenv_create
------------------

Overview
^^^^^^^^

Creates a Python virtual environment for use with a specific set of requirements
within the build stage.

.. code-block:: cmake

  virtualenv_create(
      [QUIET]
      [DESTINATION <folder>]
      [REQUIREMENTS <requirements>]
      [OUTPUT <out files>]
      [WORKING_DIRECTORY <directory>]
      [EXTRA_ARGS <args>]
  )

When using the virtualenv as a dependency in a custom target then make sure to depend
on the python executable ${envdir}/bin/python to ensure that the setup command is run first.

Example
^^^^^^^

.. code-block:: cmake

    set(pyenv ${CMAKE_CURRENT_BINARY_DIR}/pyenv)
    virtualenv_create(
        DESTINATION ${pyenv}
        REQUIREMENTS requirements.txt
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        OUTPUT module.py
    )
    add_custom_command(
        COMMAND ${pyenv}/bin/python some/script.py
        DEPENDS ${pyenv}/bin/python
        OUTPUT ...
    )

#]=======================================================================]
function(virtualenv_create)
    set(options)
    set(oneValueArgs DESTINATION REQUIREMENTS WORKING_DIRECTORY)
    set(multiValueArgs OUTPUT EXTRA_ARGS)

    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    cmake_parse_arguments(PYTHON_VENV "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if (NOT PYTHON_VENV_DESTINATION)
        message(FATAL_ERROR "DESTINATION parameter must be supplied")
    endif()

    if (NOT PYTHON_VENV_REQUIREMENTS)
        message(FATAL_ERROR "REQUIREMENTS parameter must be supplied")
    endif()

    if (NOT PYTHON_VENV_WORKING_DIRECTORY)
        message(FATAL_ERROR "WORKING_DIRECTORY parameter must be supplied")
    endif()

    if (NOT EXISTS ${PYTHON_VENV_REQUIREMENTS})
        message(FATAL_ERROR "REQUIREMENTS must exist, invalid path: ${PYTHON_VENV_REQUIREMENTS}")
    endif()

    set(PYTHON_VENV_INTERPRETER ${PYTHON_VENV_DESTINATION}/bin/python)

    add_custom_command(
        OUTPUT ${PYTHON_VENV_INTERPRETER}
        COMMAND ${Python3_EXECUTABLE} -m venv ${PYTHON_VENV_DESTINATION}
        COMMAND ${PYTHON_VENV_INTERPRETER} -m pip install --upgrade pip
        COMMAND  ${PYTHON_VENV_INTERPRETER} -m pip install -r ${PYTHON_VENV_REQUIREMENTS}
        WORKING_DIRECTORY ${PYTHON_VENV_WORKING_DIRECTORY}
        OUTPUT ${PYTHON_VENV_OUTPUT}
        ${PYTHON_VENV_EXTRA_ARGS}
    )

endfunction()

#[=======================================================================[.rst:
targets_get_all
------------------

Overview
^^^^^^^^

Locates all targets with in a directory and its subdirectories.

.. code-block:: cmake

  targets_get_all(
      [DIRECTORY <directory>]
      [RESULT <result>]
  )

  ``DIRECTORY``
    The ``DIRECTORY`` option specifies the directory to recursively search for targets
    from.

  ``RESULT``
    The ``RESULT`` option is required to store the results of the function.  The list
    of all targets from the specified directory and below.

#]=======================================================================]
function(targets_get_all)
    set(options)
    set(oneValueArgs RESULT DIRECTORY)
    set(multiValueArgs)
    cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if (NOT ARGS_RESULT)
        message(FATAL_ERROR "RESULT parameter must be supplied")
    endif()
    if (NOT ARGS_DIRECTORY)
        message(FATAL_ERROR "DIRECTORY parameter must be supplied")
    endif()

    get_property(subdirs DIRECTORY ${ARGS_DIRECTORY} PROPERTY SUBDIRECTORIES)
    foreach(subdir IN LISTS subdirs)
        targets_get_all(RESULT subTargets DIRECTORY ${subdir})
    endforeach()

    get_directory_property(allTargets DIRECTORY ${ARGS_DIRECTORY} BUILDSYSTEM_TARGETS)
    set(${ARGS_RESULT} ${subTargets} ${allTargets} PARENT_SCOPE)
endfunction()

#[=======================================================================[.rst:
targets_filter_for_sources
------------------

Overview
^^^^^^^^

Given a list of targets will return a list of targets containing sources to be
compiled.

.. code-block:: cmake

  targets_filter_for_sources(
      [TARGETS <targets...>]
      [RESULT <result>]
  )

  ``TARGETS``
    The ``TARGETS`` option specifies a list of targets to retrieve source files for.

  ``RESULT``
    The ``RESULT`` option is required to store the results of the function.  The list
    of source files for all requested targets.

#]=======================================================================]
function(targets_filter_for_sources)
    set(options)
    set(oneValueArgs RESULT)
    set(multiValueArgs TARGETS)
    cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if (NOT ARGS_TARGETS)
        message(FATAL_ERROR "TARGETS parameter must be supplied")
    endif()
    if (NOT ARGS_RESULT)
        message(FATAL_ERROR "RESULT parameter must be supplied")
    endif()

    foreach(target IN LISTS ARGS_TARGETS)
        get_target_property(targetSource ${target} SOURCES)
        if (NOT targetSource)
            continue()
        endif()

        # Interface targets can propagate sources in cmake 3.19 onward but not compile directly
        get_target_property(targetType ${target} TYPE)
        if (targetType STREQUAL "INTERFACE_LIBRARY")
            continue()
        endif()

        list(APPEND targets ${target})
    endforeach()
    set(${ARGS_RESULT} ${${ARGS_RESULT}} ${targets} PARENT_SCOPE)

endfunction()


#[=======================================================================[.rst:
targets_get_translation_units
------------------

Overview
^^^^^^^^

For a given target calculate the subset of sources which will produce a translation
unit (the compiler representation of a source file with all
marcos and include statements expanded in place).  Output is a list of resulting
sources in terms of the output location subsequent intermediate files from the
compiler are written too, i.e. a list containing the following inputs would be
transformed as such:

    ${targetSourceDir}/sourcefile.cpp -> ${targetBinaryDir}/CMakeFiles/${targetName}.dir/${file}
    ${targetSourceDir}/subdir/sourcefile.cpp -> ${targetBinaryDir}/CMakeFiles/subdir/${targetName}.dir/${file}
    ${targetBinaryDir}/sourcefile.cpp -> ${targetBinaryDir}/CMakeFiles/${targetName}.dir/${file}

#]=======================================================================]
function(targets_get_translation_units)
    set(options)
    set(oneValueArgs TARGET RESULT)
    set(multiValueArgs)
    cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if (NOT ARGS_TARGET)
        message(FATAL_ERROR "TARGET parameter must be supplied")
    endif()
    if (NOT ARGS_RESULT)
        message(FATAL_ERROR "RESULT parameter must be supplied")
    endif()

    get_target_property(targetSource ${ARGS_TARGET} SOURCES)

    # CMAKE_CXX_SOURCE_FILE_EXTENSIONS defined in: https://github.com/Kitware/CMake/blob/master/Modules/CMakeCXXCompiler.cmake.in
    foreach(cppExt IN LISTS CMAKE_CXX_SOURCE_FILE_EXTENSIONS)
        # Filter on a copy of the original source list
        set(filteredTargetSource "${targetSource}")
        # Escape any file extensions with special characters (i.e. the '+' in "c++").
        string(REGEX REPLACE "([][+.*()^])" "\\\\\\1" cppExt "${cppExt}")
        list(FILTER filteredTargetSource INCLUDE REGEX ".*\\.${cppExt}$")
        list(APPEND targetTranslationUnits ${filteredTargetSource})
    endforeach()

    get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    get_target_property(targetBinaryDir ${ARGS_TARGET} BINARY_DIR)
    foreach(file IN LISTS targetTranslationUnits)
        targets_relative_path_of_source(TARGET_NAME ${ARGS_TARGET} RESULT file SOURCE_FILE ${file})
        if(IS_MULTI_CONFIG)
            set(translationUnitLocation "${targetBinaryDir}/CMakeFiles/${ARGS_TARGET}.dir/$<CONFIG>/${file}")
        else()
            set(translationUnitLocation "${targetBinaryDir}/CMakeFiles/${ARGS_TARGET}.dir/${file}")
        endif()
        list(APPEND targetTranslationUnitLocations ${translationUnitLocation})
    endforeach()

    set(${ARGS_RESULT} ${targetTranslationUnitLocations} PARENT_SCOPE)

endfunction()


#[=======================================================================[.rst:
targets_relative_path_of_source
------------------

Overview
^^^^^^^^

#]=======================================================================]
function (targets_relative_path_of_source)
    set(options)
    set(oneValueArgs TARGET_NAME RESULT SOURCE_FILE)
    set(multiValueArgs)
    cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if (NOT ARGS_TARGET_NAME)
        message(FATAL_ERROR "TARGET_NAME parameter must be supplied")
    endif()
    if (NOT ARGS_RESULT)
        message(FATAL_ERROR "RESULT parameter must be supplied")
    endif()

    get_target_property(targetSrcDir ${ARGS_TARGET_NAME} SOURCE_DIR)
    get_target_property(targetBinaryDir ${ARGS_TARGET_NAME} BINARY_DIR)

    # Generated files should be in the subdirectories of the targets binary directory
    string(REPLACE "${targetBinaryDir}/" "" file "${ARGS_SOURCE_FILE}")

    if(IS_ABSOLUTE ${file})
        file(RELATIVE_PATH file ${targetSrcDir} ${file})
    endif()

    # get the right path for file
    string(REPLACE ".." "__" PATH "${file}")

    set(${ARGS_RESULT} "${PATH}" PARENT_SCOPE)
endfunction()


#[=======================================================================[.rst:
coverage_target_clean_intermediate_file
------------------

Overview
^^^^^^^^

#]=======================================================================]
function(coverage_target_clean_intermediate_file)
    set(options QUIET)
    set(oneValueArgs TARGET_NAME RETURN_NOTE_FILES RETURN_DATA_FILES)
    set(multiValueArgs)
    cmake_parse_arguments(COVERAGE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if (NOT COVERAGE_TARGET_NAME)
        message(FATAL_ERROR "TARGET_NAME parameter must be supplied")
    endif()

    targets_get_translation_units(TARGET ${COVERAGE_TARGET_NAME} RESULT targetSources)

    foreach (file IN LISTS targetSources)
        list(APPEND gcovNoteFiles "${file}.gcno")
        list(APPEND gcovDataFiles "${file}.gcda")
    endforeach()

    set_directory_properties(PROPERTIES ADDITIONAL_CLEAN_FILES "${gcovNoteFiles} ${gcovDataFiles}")

    if(COVERAGE_RETURN_NOTE_FILES)
        set(${COVERAGE_RETURN_NOTE_FILES} "${gcovNoteFiles}" PARENT_SCOPE)
    endif()
    if(COVERAGE_RETURN_DATA_FILES)
        set(${COVERAGE_RETURN_DATA_FILES} "${gcovDataFiles}" PARENT_SCOPE)
    endif()

endfunction()


set_property(GLOBAL PROPERTY global_coverage_test_targets)

function(add_coverage TNAME)
    message(WARNING "Adding coverage for target" ${TNAME})
    get_property(tmp GLOBAL PROPERTY global_coverage_test_targets)
    set(tmp "${tmp};${TNAME}")
    set_property(GLOBAL PROPERTY global_coverage_test_targets "${tmp}")
endfunction()


function(enable_code_coverage)
    set(options QUIET VERBOSE)
    set(oneValueArgs)
    set(multiValueArgs)

    cmake_parse_arguments(COVERAGE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    get_filename_component(COMPILER_PATH "${CMAKE_CXX_COMPILER}" PATH)
    find_program(GCOV_BIN gcov HINTS ${COMPILER_PATH})
    if(NOT GCOV_BIN)
        message(SEND_ERROR "Unable to find gcov, disable code coverage to continue")
    endif()

    find_program(LCOV_BIN lcov)
    if(NOT LCOV_BIN)
        message(SEND_ERROR "Unable to find lcov, disable code coverage to continue")
    endif()

    find_program(GENHTML_BIN genhtml)
    if(NOT GENHTML_BIN)
        message(SEND_ERROR "Unable to find genhtml, disable code coverage to continue")
    endif()

    find_program(CPPFILT_BIN c++filt)
    if(NOT CPPFILT_BIN)
        message(SEND_ERROR "Unable to find c++filt, disable code coverage to continue")
    endif()

    if (NOT COVERAGE_QUIET)
        message(STATUS "Found gcov: ${GCOV_BIN}")
        message(STATUS "Found lcov: ${LCOV_BIN}")
        message(STATUS "Found genhtml: ${GENHTML_BIN}")
    endif()

    set(COVERAGE_VENV ${CMAKE_BINARY_DIR}/.venv/coverage)
    set(COVERAGE_FASTCOV_BIN ${COVERAGE_VENV}/bin/fastcov)
    set(COVERAGE_FASTCOV_TO_SONARQUBE_BIN ${COVERAGE_VENV}/bin/fastcov_to_sonarqube)
    set(COVERAGE_LCOV_TO_COBERTURA_BIN ${COVERAGE_VENV}/bin/lcov_cobertura)
    virtualenv_create(
        DESTINATION ${COVERAGE_VENV}
        REQUIREMENTS ${PROJECT_SOURCE_DIR}/cmake/requirements/coverage_requirements.txt
        WORKING_DIRECTORY
            ${CMAKE_BINARY_DIR}
        OUTPUT
            ${COVERAGE_FASTCOV_BIN}
            ${COVERAGE_FASTCOV_TO_SONARQUBE_BIN}
            ${COVERAGE_LCOV_TO_COBERTURA_BIN}
    )

    set(COVERAGE_REPORT_DIR "${CMAKE_BINARY_DIR}/coverage")
    set(COVERAGE_REPORT_PATH "${COVERAGE_REPORT_DIR}/fastcov_report.json")
    set(SONARQUBE_REPORT_PATH "${COVERAGE_REPORT_DIR}/sonarcube_coverage.xml")
    set(COBERTURA_REPORT_PATH "${COVERAGE_REPORT_DIR}/cobertura_coverage.xml")
    set(LCOV_REPORT_PATH "${COVERAGE_REPORT_DIR}/lcov.info")
    set(LCOV_HTML_PATH "${COVERAGE_REPORT_DIR}/html")

    foreach (FLAGS ${COVERAGE_SUPPORTED_FLAGS})
        set(CMAKE_REQUIRED_FLAGS "${FLAGS}")
        check_cxx_compiler_flag("${FLAGS}" COVERAGE_FLAGS_DETECTED)

        if (COVERAGE_FLAGS_DETECTED)
            string(REPLACE " " ";" FLAGS "${FLAGS}")
            set(COVERAGE_COMPILER_FLAGS "${FLAGS}" CACHE STRING "${CMAKE_CXX_COMPILER_ID} flags for code coverage.")
            mark_as_advanced(COVERAGE_COMPILER_FLAGS)
            break()
        else ()
            message(WARNING "Code coverage is not available for the currently enable compiler ${CMAKE_CXX_COMPILER_ID} via the compiler flags ${FLAGS}.")
        endif ()
    endforeach()

    get_property(allTargets GLOBAL PROPERTY global_coverage_test_targets)
    if (COVERAGE_VERBOSE)
        message(STATUS "Coverage: All project targets: ${allTargets}")
    endif()

    targets_filter_for_sources(RESULT targetsWithSource TARGETS ${allTargets})
    if (COVERAGE_VERBOSE)
        message(STATUS "Coverage: Targets with sources: ${targetsWithSource}")
    endif()

    foreach(target IN LISTS targetsWithSource)
        target_compile_options(${target} PRIVATE ${COVERAGE_COMPILER_FLAGS})
        target_link_options(${target} PRIVATE ${COVERAGE_COMPILER_FLAGS})
        coverage_target_clean_intermediate_file(TARGET_NAME ${target} RETURN_DATA_FILES outputGCovDataFile)

        foreach(gcdaFile IN LISTS outputGCovDataFile)
            cmake_path(GET gcdaFile STEM LAST_ONLY translationUnitFile)
            get_filename_component(translationUnitDir "${gcdaFile}" DIRECTORY)
            set(translationUnit "${translationUnitDir}/${translationUnitFile}")

            set(objectFile "${translationUnit}.o")
            add_custom_command(OUTPUT "${translationUnit}.gcno"
			    COMMAND ${CMAKE_COMMAND} -E touch "${translationUnit}.gcno"
                DEPENDS "${objectFile}"
		    )
            add_custom_command(OUTPUT ${gcdaFile}
			    COMMAND ${CMAKE_COMMAND} -E touch "${gcdaFile}"
                DEPENDS "${translationUnit}.gcno"
		    )
        endforeach()
        add_custom_target(${target}-gcda-init DEPENDS ${outputGCovDataFile})

        list(APPEND gcdaInitTargets ${target}-gcda-init)
        list(APPEND allGcovDataFiles ${outputGCovDataFile})
    endforeach()

    list(REMOVE_DUPLICATES allGcovDataFiles)
    file(MAKE_DIRECTORY ${COVERAGE_REPORT_DIR})

    add_custom_target(coverage-clean
        COMMAND
            ${COVERAGE_FASTCOV_BIN}
                --gcov ${GCOV_BIN}
                --exclude ${CMAKE_BINARY_DIR}
                --include ${PROJECT_SOURCE_DIR}
                --zerocounters
        DEPENDS
            ${COVERAGE_FASTCOV_BIN}
        WORKING_DIRECTORY
            ${CMAKE_BINARY_DIR}
        COMMENT
            "Recursively delete all gcda files"
    )

    add_custom_target(coverage-fastcov
        COMMAND
            ${COVERAGE_FASTCOV_BIN}
                --gcov ${GCOV_BIN}
                --include ${PROJECT_SOURCE_DIR}
                -o ${COVERAGE_REPORT_PATH}
        DEPENDS
            ${COVERAGE_FASTCOV_BIN}
            ${gcdaInitTargets}
        WORKING_DIRECTORY
            ${CMAKE_BINARY_DIR}
        COMMENT
            "Distributed processing of coverage data collection and report generation"
    )

    add_custom_target(coverage)
    add_dependencies(coverage coverage-fastcov)

    add_custom_command(
        OUTPUT ${LCOV_REPORT_PATH}
        COMMAND
            ${COVERAGE_FASTCOV_BIN}
                --gcov ${GCOV_BIN}
                --exclude ${CMAKE_CURRENT_BINARY_DIR}
                --include ${PROJECT_SOURCE_DIR}
                --lcov
                --verbose
                -o ${LCOV_REPORT_PATH}
        DEPENDS
            ${COVERAGE_FASTCOV_BIN}
            ${LCOV_BIN}
            ${allGcovDataFiles}
            # Missing source file (*.cpp and *.gcda) dependencies.
        WORKING_DIRECTORY
            ${CMAKE_BINARY_DIR}
        COMMENT
            "Distributed processing of coverage data collection and lcov report generation"
    )

    add_custom_target(coverage-lcov-info
        DEPENDS ${LCOV_REPORT_PATH}
    )

    add_custom_command(
        OUTPUT ${LCOV_HTML_PATH}/index.html
        COMMAND ${GENHTML_BIN} -o ${LCOV_HTML_PATH} ${LCOV_REPORT_PATH} --demangle-cpp
        DEPENDS ${GEMHTML_BIN} ${LCOV_REPORT_PATH}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating lcov html report"
    )

    add_custom_target(coverage-lcov
        DEPENDS ${LCOV_HTML_PATH}/index.html
    )

    add_dependencies(coverage-lcov coverage-lcov-info)

    add_custom_command(
        OUTPUT ${COBERTURA_REPORT_PATH}
        COMMAND
            ${COVERAGE_LCOV_TO_COBERTURA_BIN} ${LCOV_REPORT_PATH}
                --base-dir ${PROJECT_SOURCE_DIR}
                --output ${COBERTURA_REPORT_PATH}
                --demangle
        DEPENDS
            ${LCOV_REPORT_PATH}
            ${COVERAGE_LCOV_TO_COBERTURA_BIN}
        WORKING_DIRECTORY
            ${CMAKE_BINARY_DIR}
        COMMENT
            "Generate Cobertura XML report from LCov report"
    )

    add_custom_target(coverage-cobertura
        DEPENDS ${COBERTURA_REPORT_PATH}
    )

endfunction()
