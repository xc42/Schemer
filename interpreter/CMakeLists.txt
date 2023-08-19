cmake_minimum_required(VERSION 3.0.0)

file(GLOB parser "../parser/*.cpp")
file(GLOB VM    "../VM/*.cpp")
add_executable(schemer interpreter.cpp REPL.cpp ${parser} ${VM})
target_include_directories(schemer PUBLIC ../common ../parser ../VM)
target_link_libraries(schemer fmt::fmt)
target_compile_options(schemer PUBLIC -fno-omit-frame-pointer)
