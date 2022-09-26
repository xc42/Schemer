cmake_minimum_required(VERSION 3.0.0)

find_package(LLVM REQUIRED CONFIG)

file(GLOB parser "../parser/*.cpp")
add_executable(sch-c codegen.cpp main.cpp ${parser})

target_include_directories(sch-c PUBLIC ../common ../parser ${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs support core)
target_link_libraries(sch-c fmt::fmt ${llvm_libs})
