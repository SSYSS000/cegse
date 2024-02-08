cmake_minimum_required(VERSION 2.8...3.5)

project(cegse)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE MATCHES Release)
   set(CMAKE_C_FLAGS_RELEASE "-Os -DNDEBUG")
elseif(CMAKE_BUILD_TYPE MATCHES Debug)
   set(CMAKE_C_FLAGS_DEBUG "-Og -g")
endif()

add_compile_options(-Wall -Wextra)

add_executable(${PROJECT_NAME}
    src/main.c
    $<TARGET_OBJECTS:dependencies>
)

add_library(dependencies STATIC 
    src/compression.c
    src/defines.h
    src/log.h
    src/log.c
    src/compression.h
    src/binary_stream.h
    src/endianness.h
    src/binary_stream.c
    src/savefile.h
    src/savefile.c
    src/mem_types.c
    src/mem_types.h
    src/mem_type_casts.h
)

target_link_libraries(${PROJECT_NAME} lz4 z)

include(CTest)

# Unit test files
set(unit_test_files
    src/savefile.c
    src/binary_stream.c
)

foreach(file ${unit_test_files})
    get_filename_component(target ${file} NAME_WLE)
    set(target unit_tests_${target})

    # Unit test release build with asserts disabled.
    set(CMAKE_BUILD_TYPE Release)
    add_executable(${target} ${file})

    # To shut GCC up about sign comparison when an operand's type
    # has been acquired by passing an enum constant to typeof().
    target_compile_options(${target} PRIVATE -Wno-sign-compare)

    # Compile with unit test specific code.
    target_compile_definitions(${target} PRIVATE COMPILE_WITH_UNIT_TESTS=1)

    target_link_libraries(${target} dependencies lz4 z)

    add_test(NAME ${target} COMMAND ${target})
endforeach()
