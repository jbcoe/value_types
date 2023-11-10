include_guard(GLOBAL)

if (ENABLE_SANITIZERS)
    set(SANITIZER_FLAGS_ASAN "-fsanitize=address -fno-omit-frame-pointer")
    set(SANITIZER_FLAGS_UBSAN "-fsanitize=undefined")

    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("${SANITIZER_FLAGS_ASAN}" COMPILER_SUPPORTS_ASAN)
    check_cxx_compiler_flag("${SANITIZER_FLAGS_UBSAN}" COMPILER_SUPPORTS_UBSAN)

    if (COMPILER_SUPPORTS_ASAN)
        add_library(asan INTERFACE IMPORTED)
        set_target_properties(asan PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${SANITIZER_FLAGS_ASAN}"
            INTERFACE_LINK_OPTIONS "${SANITIZER_FLAGS_ASAN}"
         )
    endif(COMPILER_SUPPORTS_ASAN)

    if (COMPILER_SUPPORTS_UBSAN)
        add_library(ubsan INTERFACE IMPORTED)
        set_target_properties(ubsan PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${SANITIZER_FLAGS_UBSAN}"
            INTERFACE_LINK_OPTIONS "${SANITIZER_FLAGS_UBSAN}"
        )
    endif(COMPILER_SUPPORTS_UBSAN)
endif(ENABLE_SANITIZERS)
