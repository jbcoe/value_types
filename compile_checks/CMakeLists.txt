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

add_library(indirect_requirements OBJECT)
target_sources(indirect_requirements
    PRIVATE
        indirect_requirements.cc
)
target_link_libraries(indirect_requirements
    PRIVATE
        indirect
        common_compiler_settings
)

if(NOT MSVC)
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
endif()

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

add_library(polymorphic_requirements OBJECT)
target_sources(polymorphic_requirements
    PRIVATE
        polymorphic_requirements.cc
)
target_link_libraries(polymorphic_requirements
    PRIVATE
        polymorphic
        common_compiler_settings
)
