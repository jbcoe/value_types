vt_add_library(
    NAME indirect_consteval
    ALIAS value_types::indirect_consteval
)
target_sources(indirect_consteval
    PRIVATE
        indirect_consteval.cc
)
target_link_libraries(indirect_consteval
    PRIVATE
        indirect
        common_compiler_settings
)

vt_add_library(
    NAME indirect_pimpl
    ALIAS value_types::indirect_pimpl
)
target_sources(indirect_pimpl
    PRIVATE
        indirect_pimpl.h
        indirect_pimpl.cc
        indirect_pimpl_use.cc
)
target_link_libraries(indirect_pimpl
    PRIVATE
        indirect
        common_compiler_settings
)

if(NOT MSVC)
    vt_add_library(
        NAME polymorphic_consteval
        ALIAS value_types::polymorphic_consteval
    )
    add_library(polymorphic_consteval "")
    target_sources(polymorphic_consteval
        PRIVATE
            polymorphic_consteval.cc
    )
    target_link_libraries(polymorphic_consteval
        PRIVATE
            polymorphic
            common_compiler_settings
    )
endif()

vt_add_library(
    NAME polymorphic_pimpl
    ALIAS value_types::polymorphic_pimpl
)
target_sources(polymorphic_pimpl
    PRIVATE
        polymorphic_pimpl.h
        polymorphic_pimpl.cc
        polymorphic_pimpl_use.cc
)
target_link_libraries(polymorphic_pimpl
    PRIVATE
        polymorphic
        common_compiler_settings
)
