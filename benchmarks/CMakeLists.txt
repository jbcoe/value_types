add_executable(indirect_benchmark "")
target_sources(indirect_benchmark
    PRIVATE
        indirect_benchmark.cc
)
target_link_libraries(indirect_benchmark
    PRIVATE
        polymorphic
        benchmark::benchmark_main
        common_compiler_settings
)

add_executable(polymorphic_benchmark "")
target_sources(polymorphic_benchmark
    PRIVATE
        polymorphic_benchmark.cc
)
target_link_libraries(polymorphic_benchmark
    PRIVATE
        polymorphic
        benchmark::benchmark_main
        common_compiler_settings
)
