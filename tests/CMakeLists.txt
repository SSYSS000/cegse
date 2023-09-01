cmake_minimum_required(VERSION 2.8...3.5)

include_directories(../src)

add_library(test_modules SHARED
    ../src/compression.c
    ../src/binary_stream.c
    ../src/savefile.c
    ../src/log.c
    ../src/mem_types.c
)

target_link_libraries(test_modules lz4 z)

file(GLOB files "test_*.c")

foreach(file ${files})
    get_filename_component(target ${file} NAME_WLE)
    add_executable(${target} ${file})
    target_compile_options(${target} PRIVATE -Wno-parentheses)
    target_link_libraries(${target} test_modules)
    add_test(NAME ${target} COMMAND ${target})
endforeach()

