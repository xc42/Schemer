cmake_minimum_required(VERSION 3.0.0)

project(SchemePlayground VERSION 0.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(fmt)
add_subdirectory(interpreter)
add_subdirectory(compiler)
