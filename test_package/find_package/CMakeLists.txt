cmake_minimum_required(VERSION 3.22)
project(test_find_package)

find_package(xyz_value_types REQUIRED)

add_executable(main ../src/main.cc)
target_compile_features(main PRIVATE cxx_std_20)
target_link_libraries(main PRIVATE xyz_value_types::value_types)
