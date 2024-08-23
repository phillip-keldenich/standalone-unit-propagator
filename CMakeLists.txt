cmake_minimum_required(VERSION 3.16..3.30)
project(standalone_propagator CXX)
include_directories("${CMAKE_CURRENT_LIST_DIR}/include")
set(CMAKE_CXX_STANDARD 20)
enable_testing()
add_subdirectory(test)

