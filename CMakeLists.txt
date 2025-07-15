cmake_minimum_required(VERSION 3.17)
project(Stratum LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(stratum INTERFACE)
target_include_directories(stratum INTERFACE src)

add_executable(example examples/example.cpp)

target_link_libraries(example PRIVATE stratum)
