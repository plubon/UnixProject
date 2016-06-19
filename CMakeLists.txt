cmake_minimum_required(VERSION 3.5)
project(UnixProject)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.c utils.h utils.c)
add_executable(UnixProject ${SOURCE_FILES})
target_link_libraries(UnixProject pthread)
target_link_libraries(UnixProject rt)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set_property(TARGET UnixProject PROPERTY C_STANDARD 99)