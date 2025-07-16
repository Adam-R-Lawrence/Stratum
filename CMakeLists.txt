cmake_minimum_required(VERSION 3.21)
project(Stratum LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(stratum INTERFACE)
target_include_directories(stratum INTERFACE src)

add_executable(stratum_example_parse examples/parse_gcode_test.cpp)
add_executable(stratum_example_generate examples/generate_gcode_test.cpp)

target_link_libraries(stratum_example_parse PRIVATE stratum)
target_link_libraries(stratum_example_generate PRIVATE stratum)