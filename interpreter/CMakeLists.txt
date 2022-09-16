cmake_minimum_required(VERSION 3.0.0)

file(GLOB parser "../parser/*.cpp")
add_executable(repl interpreter.cpp REPL.cpp ${parser})
target_include_directories(repl PUBLIC ../common ../parser)
target_link_libraries(repl fmt::fmt)
