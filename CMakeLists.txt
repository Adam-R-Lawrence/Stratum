cmake_minimum_required(VERSION 3.21)
project(Stratum LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(stratum INTERFACE)
target_include_directories(stratum INTERFACE src)

enable_testing()

add_executable(test_parse_gcode tests/test_parse_gcode.cpp)
target_link_libraries(test_parse_gcode PRIVATE stratum)
add_test(NAME parse_gcode COMMAND test_parse_gcode)

add_executable(test_generate_gcode tests/test_generate_gcode.cpp)
target_link_libraries(test_generate_gcode PRIVATE stratum)
add_test(NAME generate_gcode COMMAND test_generate_gcode)

add_executable(test_parse_photocuring_gcode tests/test_parse_photocuring_gcode.cpp)
target_link_libraries(test_parse_photocuring_gcode PRIVATE stratum)
add_test(NAME parse_photocuring_gcode COMMAND test_parse_photocuring_gcode)
