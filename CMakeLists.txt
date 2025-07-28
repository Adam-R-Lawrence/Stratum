# CMakeLists.txt  (only the changed / added lines are commented)

cmake_minimum_required(VERSION 3.21)
project(Stratum LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(lodepng STATIC external/lodepng/lodepng.cpp)
target_include_directories(lodepng PUBLIC external/lodepng)

add_library(stratum INTERFACE)
target_include_directories(stratum INTERFACE src)
target_link_libraries(stratum INTERFACE lodepng)