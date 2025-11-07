if (${XYZ_VALUE_TYPES_IS_NOT_SUBPROJECT})
    if (${BUILD_TESTING})
        xyz_add_test(
            NAME incomplete_types_test
            FILES
                incomplete_types.h
                incomplete_types.cc
                incomplete_types_test.cc
        )

        xyz_add_test(
            NAME recursive_variant_test
            LINK_LIBRARIES indirect
            FILES
                recursive_variant.h
                recursive_variant.cc
                recursive_variant_test.cc
        )

        add_library(traits_and_concepts OBJECT)
        target_sources(traits_and_concepts
            PRIVATE
                traits_and_concepts.cc
        )
        set_target_properties(traits_and_concepts PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS NO
        )

    endif()
endif()
