add_library(indirect_consteval OBJECT)
target_sources(indirect_consteval
    PRIVATE
        indirect_consteval.cc
)
target_link_libraries(indirect_consteval
    PRIVATE
        indirect
        common_compiler_settings
)

add_library(indirect_pimpl OBJECT)
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

add_library(polymorphic_consteval OBJECT)
target_sources(polymorphic_consteval
    PRIVATE
        polymorphic_consteval.cc
)
target_link_libraries(polymorphic_consteval
    PRIVATE
        polymorphic
        common_compiler_settings
)

add_library(polymorphic_pimpl OBJECT)
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

add_library(indirect_as_member OBJECT)
target_sources(indirect_as_member
    PRIVATE
        indirect_as_member.cc
)
target_link_libraries(indirect_as_member
    PRIVATE
        indirect
        common_compiler_settings
)

add_library(indirect_in_variant OBJECT)
target_sources(indirect_in_variant
    PRIVATE
        indirect_in_variant.cc
)
target_link_libraries(indirect_in_variant
    PRIVATE
        indirect
        common_compiler_settings
)

add_library(polymorphic_as_member OBJECT)
target_sources(polymorphic_as_member
    PRIVATE
        polymorphic_as_member.cc
)
target_link_libraries(polymorphic_as_member
    PRIVATE
        polymorphic
        common_compiler_settings
)

add_library(polymorphic_in_variant OBJECT)
target_sources(polymorphic_in_variant
    PRIVATE
        polymorphic_in_variant.cc
)
target_link_libraries(polymorphic_in_variant
    PRIVATE
        polymorphic
        common_compiler_settings
)
