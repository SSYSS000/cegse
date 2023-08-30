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
    src/compression.c
    src/defines.h
    src/log.h
    src/compression.h
    src/binary_stream.h
    src/endian.h
    src/binary_stream.c
    src/main.c
    src/savefile.h
    src/savegame.h
    src/savefile.c
    src/savegame.c
)

target_link_libraries(${PROJECT_NAME} lz4 z)

include(CTest)
add_subdirectory(tests)
