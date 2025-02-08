if (${XYZ_VALUE_TYPES_IS_NOT_SUBPROJECT})
    if (${BUILD_TESTING})
        xyz_add_test(
            NAME incomplete_types_test
            FILES
                incomplete_types.h
                incomplete_types.cc
                incomplete_types_test.cc
        )
    endif()
endif()
