cmake_minimum_required(VERSION 3.22)
project(test_fetchcontent)

include(FetchContent)

FetchContent_Declare(
    xyz_value_types
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../..
)
FetchContent_MakeAvailable(xyz_value_types)

add_executable(main ../src/main.cc)
target_compile_features(main PRIVATE cxx_std_20)
target_link_libraries(main PRIVATE xyz_value_types::value_types)
