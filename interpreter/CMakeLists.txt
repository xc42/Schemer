cmake_minimum_required(VERSION 3.0.0)

file(GLOB parser "../parser/*.cpp")
add_executable(schemer interpreter.cpp REPL.cpp ${parser})
target_include_directories(schemer PUBLIC ../common ../parser)
target_link_libraries(schemer fmt::fmt)
